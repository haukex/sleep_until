from setuptools import Extension, setup

setup(
    ext_modules = [ Extension( 'sleep_until', sources = ['sleep_until.c'] ) ]
)
