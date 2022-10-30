import argparse
import sys
from pathlib import Path
import zipfile
import tarfile

CWD      = Path(__file__).parent
ROOT_DIR = CWD / ".."

def package(lib: Path, out: Path, toolchain: str):
    ext = "".join(out.suffixes)
    libname = "omvll_{}{}".format(toolchain, lib.suffix)

    if ext == ".zip":
        with zipfile.ZipFile(out, mode='w', compression=zipfile.ZIP_DEFLATED) as outzip:
            outzip.write(lib, libname)
            outzip.testzip()
        return 0

    if ext == ".tar.gz":
        with tarfile.open(out, mode='w:gz') as outtar:
            outtar.add(lib, libname)
        return 0

    print(f"Unknown format ({ext})")
    sys.exit(1)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--toolchain", '-t', default="unknown")
    parser.add_argument("OMVLL")
    parser.add_argument("output")

    args = parser.parse_args()
    package(Path(args.OMVLL), Path(args.output), args.toolchain)
    return 0

if __name__ == "__main__":
    sys.exit(main())
