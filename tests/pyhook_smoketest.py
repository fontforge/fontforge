#!/usr/bin/env python
from __future__ import print_function

import fontforge
import psMat
import pickle

print(fontforge.__version__, fontforge.version())

fontforge.font()

print(pickle.loads(pickle.dumps(fontforge.point())))
