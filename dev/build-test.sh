#!/bin/bash
set -euxo pipefail

usage() { echo "Usage: $0 PYTHON3BIN" 1>&2; exit 1; }
[[ $# -eq 1 ]] || usage
if [[ "$1" != /* ]]; then
    python3bin="$PWD/$1"
else
    python3bin="$1"
fi

cd -- "$( dirname -- "${BASH_SOURCE[0]}" )"/..

rm -rf dist
"$python3bin" -m build
PYTHON3BIN="$python3bin" dev/isolated-dist-test.sh dist/*.tar.gz
