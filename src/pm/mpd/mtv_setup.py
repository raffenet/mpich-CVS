#!/usr/bin/env python

from distutils.core import setup, Extension

mtv = Extension("mtv",["mtv.c"])

setup(name="mtv", version="1.0", ext_modules=[mtv])
