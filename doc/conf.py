#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import os
import sphinx
import sys
from typing import Any
from datetime import date
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective
from sphinx.util.console import red
from docutils import nodes

logger = logging.getLogger(__name__)

try:
    import omvll
except ImportError:
    OMVLL_DIR = os.getenv("OMVLL_STANDALONE_DIR", None)
    if OMVLL_DIR is None:
        print(red("""\
        Please define 'OMVLL_STANDALONE_DIR' or 'PYTHONPATH'
        to the OMVLL standalone directory
        """))
        sys.exit(1)
    sys.path.insert(0, OMVLL_DIR)
    import omvll

# -- Env  -----------------------------------------------------
OPEN_OBF_BASE = "https://obfuscator.re"
OMVLL_BASE    = "https://open-obfuscator.github.io/o-mvll"

# -- Roles  -----------------------------------------------------
def omvll_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    url = f"{OPEN_OBF_BASE}/omvll/passes/{text}"
    pass_link = nodes.reference("", text, refuri=url, **options)
    return [pass_link], []

def dprotect_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    url = f"{OPEN_OBF_BASE}/dprotect/passes/{text}"
    pass_link = nodes.reference("", text, refuri=url, **options)
    return [pass_link], []


class Alignment(sphinx.transforms.SphinxTransform):
    default_priority = 401
    def apply(self, **kwargs: Any) -> None:
        # Fix table alignment
        for table in self.document.findall(nodes.table):
            table.setdefault("align", "left")

class CustomSubstitutions(sphinx.transforms.SphinxTransform):
    default_priority = 1
    substitutions = {
        'omvll-passes',
    }

    def apply(self, **kwargs: Any) -> None:
        for ref in self.document.findall(nodes.substitution_reference):
            refname = ref['refname']
            if refname in CustomSubstitutions.substitutions:
                if refname == "omvll-passes":
                    passlist = nodes.enumerated_list()
                    for name in omvll.config.passes:
                        passlist += nodes.list_item('', nodes.Text(name))
                    ref.replace_self(passlist)
                    continue



def setup(app: sphinx.application.Sphinx):
    app.add_role('omvll',    omvll_role)
    app.add_role('dprotect', dprotect_role)
    app.add_transform(Alignment)
    app.add_transform(CustomSubstitutions)

# -- Project information -----------------------------------------------------
date          = date.today()
project       = 'O-MVLL'
html_title    = "O-MVLL Documentation"
copyright     = f'{date.year}, Romain Thomas'
author        = 'Romain Thomas'
source_suffix = '.rst'
master_doc    = 'index'

version = omvll.OMVLL_VERSION
release = version


# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.mathjax',
    'sphinx.ext.autodoc',
]

templates_path = ['_templates']

exclude_patterns = []

autoclass_content = 'both'


# -- Options for HTML output -------------------------------------------------

html_baseurl     = OMVLL_BASE
html_theme       = 'furo'
html_copy_source = False
html_static_path = ['_static']
html_logo        = "_static/logo.webp"
html_favicon     = '_static/favicon.png'
html_theme_options = {
}
