#!/usr/bin/env python
from __future__ import print_function
import sys

print(sys.path)
import fontforge
import psMat
import pickle
import sysconfig

print(sysconfig.get_paths())
if hasattr(sysconfig, "_get_preferred_schemes"):
    print(sysconfig._get_preferred_schemes())

print(fontforge.__version__, fontforge.version())

fontforge.font()

print(pickle.loads(pickle.dumps(fontforge.point())))
