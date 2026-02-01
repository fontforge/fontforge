#!/usr/bin/env python
from __future__ import print_function
import os
import sys

# On Windows, add DLL search paths before importing fontforge
if sys.platform == 'win32':
    # Add the directory containing fontforge.pyd to DLL search path
    script_dir = os.path.dirname(os.path.abspath(__file__))
    lib_dir = os.environ.get('PYTHONPATH', '').split(os.pathsep)[0]
    if lib_dir and os.path.isdir(lib_dir):
        os.add_dll_directory(os.path.abspath(lib_dir))

import fontforge
import psMat
import pickle

print(fontforge.__version__, fontforge.version())

fontforge.font()

print(pickle.loads(pickle.dumps(fontforge.point())))
