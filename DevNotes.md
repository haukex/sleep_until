
References:

- <https://packaging.python.org/en/latest/tutorials/packaging-projects/>
- <https://setuptools.pypa.io/en/latest/userguide/quickstart.html>
- <https://setuptools.pypa.io/en/latest/userguide/ext_modules.html>
- <https://packaging.python.org/en/latest/guides/using-testpypi/#using-testpypi-with-pip>
- <https://cibuildwheel.readthedocs.io/en/stable/setup/#github-actions>

Initial setup:

    pip3 install --upgrade build twine

Building:

    python3 -m build

The Windows `wheel`s are built by GitHub Actions using `cibuildwheel`.
When the build has completed, the `artifact.zip` can be downloaded from GitHub
and unpacked into the `dist` directory.

Uploading to test repo:

    twine upload --repository testpypi dist/sleep_until-*.tar.gz dist/sleep_until-*-win*.whl

Testing (note in the first `pip3` command, version can be specified by e.g. `sleep_until==1.0`):

    pip3 install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ sleep_until
    # - OR -
    pip3 install dist/sleep_until-*.tar.gz
    # then
    python3 test.py 30

Uploading to PyPI:

    twine upload dist/sleep_until-*.tar.gz dist/sleep_until-*-win*.whl

Cleaning:

    rm -rf dist sleep_until.egg-info
    pip3 uninstall -y sleep_until
    pip3 cache remove sleep_until
