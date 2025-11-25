
References:

- <https://packaging.python.org/en/latest/tutorials/packaging-projects/>
- <https://setuptools.pypa.io/en/latest/userguide/quickstart.html>
- <https://setuptools.pypa.io/en/latest/userguide/ext_modules.html>
- <https://packaging.python.org/en/latest/guides/using-testpypi/#using-testpypi-with-pip>
- <https://cibuildwheel.readthedocs.io/en/stable/setup/#github-actions>

Initial setup:

    make installdeps

Building (Linux):

    python -m build

Local Test:

    dev/isolated-dist-test.sh dist/sleep_until-*.tar.gz

Clean:

    git clean -dxf -e '.venv*'

The Windows `wheel`s are built by GitHub Actions using `cibuildwheel`.
When the build has completed, the artifact's `.zip` can be downloaded
from GitHub and unpacked into the `dist` directory.
