#!/usr/bin/env python3
import sys
import os
import logging
import pathlib
from mako.template import Template
from enum import Enum, auto
import tempfile
import boto3
from botocore.exceptions import ClientError
from datetime import datetime

class CI(Enum):
    UNKNOWN        = auto()
    GITHUB_ACTIONS = auto()

def pretty_ci_name(ci):
    return str(ci).split(".")[-1].replace("_", "-").lower()

def is_pr(ci):
    if ci == CI.GITHUB_ACTIONS:
        cond1 = os.getenv("GITHUB_HEAD_REF", "") != ""
        cond2 = not (os.getenv("GITHUB_REPOSITORY", "").startswith("open-obfuscator/") or os.getenv("GITHUB_REPOSITORY", "").startswith("romainthomas/"))
        return cond1 or cond2
    return True

def get_branch(ci):
    if ci == CI.GITHUB_ACTIONS:
        return os.getenv("GITHUB_REF").replace("refs/heads/", "")
    return None

def get_ci_workdir(ci):
    if ci == CI.GITHUB_ACTIONS:
        return os.getenv("GITHUB_WORKSPACE")
    logger.critical("Unsupported CI to resolve working directory")
    sys.exit(1)

def get_tag(ci):
    if ci == CI.GITHUB_ACTIONS:
        ref = os.getenv("GITHUB_REF", "")
        logger.info("Github Action tag: {}".format(ref))
        if ref.startswith("refs/tags/"):
            return ref.replace("refs/tags/", "")
        return ""

    logger.critical("Unsupported CI to resolve working directory")
    sys.exit(1)

LOG_LEVEL = logging.INFO

logging.getLogger().addHandler(logging.StreamHandler(stream=sys.stdout))
logging.getLogger().setLevel(LOG_LEVEL)
logger = logging.getLogger(__name__)

CURRENT_CI = CI.GITHUB_ACTIONS

CI_PRETTY_NAME = pretty_ci_name(CURRENT_CI)
logger.info("CI: %s", CI_PRETTY_NAME)

ALLOWED_BRANCHES = {"main", "deploy", "devel"}
BRANCH_NAME      = get_branch(CURRENT_CI)
TAG_NAME         = get_tag(CURRENT_CI)
IS_TAGGED        = TAG_NAME is not None and len(TAG_NAME) > 0

logger.info("Branch: %s", BRANCH_NAME)
logger.info("Tag:    %s", TAG_NAME)

if BRANCH_NAME.startswith("release-"):
    logger.info("Branch release")
elif BRANCH_NAME not in ALLOWED_BRANCHES and not IS_TAGGED:
    logger.info("Skip deployment for branch '%s'", BRANCH_NAME)
    sys.exit(0)

if is_pr(CURRENT_CI):
    logger.info("Skip pull request")
    sys.exit(0)

CURRENTDIR = pathlib.Path(__file__).resolve().parent
REPODIR    = CURRENTDIR / ".." / ".."

# According to Scaleway S3 documentation, the endpoint
# should starts with '<bucket>'.s3.<region>.scw.cloud
# Nevertheless boto3 uses /{Bucket} endpoints suffix
# which create issues (see: https://stackoverflow.com/a/70383653)
OMVLL_S3_REGION   = "fr-par"
OMVLL_S3_ENDPOINT = "https://s3.{region}.scw.cloud".format(region=OMVLL_S3_REGION)
OMVLL_S3_BUCKET   = "obfuscator"
OMVLL_S3_KEY      = os.getenv("OMVLL_S3_KEY", None)
OMVLL_S3_SECRET   = os.getenv("OMVLL_S3_SECRET", None)

BUILD38_S3_REGION   = "eu-central-1"
BUILD38_S3_ENDPOINT = "https://s3.{region}.amazonaws.com".format(region=BUILD38_S3_REGION)
BUILD38_S3_BUCKET   = "build38-open-obfuscator"
BUILD38_S3_KEY      = os.getenv("BUILD38_S3_KEY", None)
BUILD38_S3_SECRET   = os.getenv("BUILD38_S3_SECRET", None)

BINARIES_CONTENT_TYPE = "binary/octet-stream"
INDEX_CONTENT_TYPE = "text/html"

omvll_keys_set = True
build38_keys_set = True

if (OMVLL_S3_KEY is None or len(OMVLL_S3_KEY) == 0) or (OMVLL_S3_SECRET is None or len(OMVLL_S3_SECRET) == 0):
    logger.error("OMVLL_S3_KEY/OMVLL_S3_SECRET not set!")
    omvll_keys_set = False

if (BUILD38_S3_KEY is None or len(BUILD38_S3_KEY) == 0) or (BUILD38_S3_SECRET is None or len(BUILD38_S3_SECRET) == 0):
    logger.error("BUILD38_S3_KEY/BUILD38_S3_SECRET not set!")
    build38_keys_set = False

if not omvll_keys_set and not build38_keys_set:
    sys.exit(1)

CI_CWD = pathlib.Path(get_ci_workdir(CURRENT_CI))

if CI_CWD is None:
    logger.error("Can't resolve CI working dir")
    sys.exit(1)

DIST_DIR  = REPODIR / "dist"

logger.info("Working directory: %s", CI_CWD)

INDEX_TEMPLATE = r"""<!DOCTYPE html>
<html>
<title>Packages for O-MVLL</title>
<body>
<h1>Packages for O-MVLL</h1>
% for path, filename in files:
    <a href="/${path}">${filename}</a><br />
% endfor
</body>
</html>
"""

SKIP_LIST = ["index.html"]

omvll_s3 = boto3.resource(
    's3',
    region_name=OMVLL_S3_REGION,
    use_ssl=True,
    endpoint_url=OMVLL_S3_ENDPOINT,
    aws_access_key_id=OMVLL_S3_KEY,
    aws_secret_access_key=OMVLL_S3_SECRET
)

build38_s3 = boto3.resource(
    's3',
    region_name=BUILD38_S3_REGION,
    use_ssl=True,
    endpoint_url=BUILD38_S3_ENDPOINT,
    aws_access_key_id=BUILD38_S3_KEY,
    aws_secret_access_key=BUILD38_S3_SECRET
)

def push(file: str, dir_name: str, s3_bucket_name: str, s3_resource, s3_content_type: str, use_timestamp=True):
    zipfile = pathlib.Path(file)
    if use_timestamp:
        now = datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
        dst = f"{dir_name}/{zipfile.stem}_{now}{zipfile.suffix}"
    else:
        dst = f"{dir_name}/{zipfile.name}"
    logger.info("Uploading %s to %s", file, dst)
    try:
        obj = s3_resource.Object(s3_bucket_name, dst)
        obj.put(Body=zipfile.read_bytes(), ContentType=s3_content_type)
        return 0
    except ClientError as e:
        logger.error("S3 push failed: %s", e)
        return 1

def filename(object):
    return pathlib.Path(object.key).name

def generate_index(dir_name: str, s3_bucket_name: str, s3_resource):
    files = s3_resource.Bucket(s3_bucket_name).objects.filter(Prefix=f'{dir_name}/omvll')
    tmpl_info = [(object.key, filename(object)) for object in files if filename(object) not in SKIP_LIST]
    html = Template(INDEX_TEMPLATE).render(files=tmpl_info)
    return html

dir_name = "latest"

if BRANCH_NAME != "main":
    dir_name = "{}".format(BRANCH_NAME.replace("/", "-").replace("_", "-"))

if BRANCH_NAME.startswith("release-"):
    _, dir_name = BRANCH_NAME.split("release-")

if IS_TAGGED:
    dir_name = str(TAG_NAME)

dir_name = f"{dir_name}/omvll" if omvll_keys_set else "ci"
logger.info("Destination directory: %s", dir_name)

for file in DIST_DIR.glob("*.zip"):
    logger.info("[ZIP   ] Uploading '%s'", file.as_posix())
    # Nightly deployment
    if omvll_keys_set:
        push(file.as_posix(), dir_name, OMVLL_S3_BUCKET, omvll_s3, BINARIES_CONTENT_TYPE)
    # Experimental deployment
    if build38_keys_set:
        push(file.as_posix(), dir_name, BUILD38_S3_BUCKET, build38_s3, BINARIES_CONTENT_TYPE)

for file in DIST_DIR.glob("*.tar.gz"):
    logger.info("[TAR.GZ] Uploading '%s'", file.as_posix())
    # Nightly deployment
    if omvll_keys_set:
        push(file.as_posix(), dir_name, OMVLL_S3_BUCKET, omvll_s3, BINARIES_CONTENT_TYPE)
    # Experimental deployment
    if build38_keys_set:
        push(file.as_posix(), dir_name, BUILD38_S3_BUCKET, build38_s3, BINARIES_CONTENT_TYPE)

if omvll_keys_set:
    nightly_index = generate_index(dir_name, OMVLL_S3_BUCKET, omvll_s3)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        index = (tmp / "index.html")
        index.write_text(nightly_index)
        push(index.as_posix(), dir_name, OMVLL_S3_BUCKET, omvll_s3, INDEX_CONTENT_TYPE, False)

if build38_keys_set:
    experimental_index = generate_index(dir_name, BUILD38_S3_BUCKET, build38_s3)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        index = (tmp / "index.html")
        index.write_text(experimental_index)
        push(index.as_posix(), dir_name, BUILD38_S3_BUCKET, build38_s3, INDEX_CONTENT_TYPE, False)

logger.info("Done!")
