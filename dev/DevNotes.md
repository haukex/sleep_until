
All of the following assume Linux.

Initial setup:

    for v in $(seq 10 14); do test -d .venv3.$v || python3.$v -m venv .venv3.$v; done
    for v in $(seq 10 14); do make installdeps PYTHON3BIN=.venv3.$v/bin/python; done

Local Test:

    for v in $(seq 10 14); do dev/build-test.sh .venv3.$v/bin/python; done

Building:

    rm -rf dist && python -m build
    tar tzvf dist/sleep_until-*.tar.gz

The Windows `wheel`s are built by GitHub Actions using `cibuildwheel`.
When the build has completed, the artifact's `.zip` can be downloaded
from GitHub and unpacked into the `dist` directory alongside the `.tar.gz`.

    python -m twine check dist/*{.tar.gz,.whl}
    python -m twine upload dist/*{.tar.gz,.whl}

Clean:

    git clean -dxf -e '.venv*'


References:

- <https://packaging.python.org/en/latest/tutorials/packaging-projects/>
- <https://setuptools.pypa.io/en/latest/userguide/quickstart.html>
- <https://setuptools.pypa.io/en/latest/userguide/ext_modules.html>
- <https://packaging.python.org/en/latest/guides/using-testpypi/#using-testpypi-with-pip>
- <https://cibuildwheel.readthedocs.io/en/stable/setup/#github-actions>

<!-- spell-checker: ignore cibuildwheel installdeps venv -->
