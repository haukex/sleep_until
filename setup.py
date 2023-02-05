from setuptools import Extension, setup

setup(
    name = "sleep_until",
    version = "1.0",
    ext_modules = [ Extension( 'sleep_until', sources = ['sleep_until.c'] ) ]
)
