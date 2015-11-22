#!/usr/bin/env python

from setuptools import setup

setup(name='python-sip-ue',
      version='0.1',
      description='Python SIP UE',
      author='Rob Day',
      author_email='rkd@rkd.me.uk',
      packages=['sipue'],
      setup_requires=["cffi>=1.0.0"],
      cffi_modules=["compile.py:ffi"],
      install_requires=["cffi>=1.0.0"],
     )
