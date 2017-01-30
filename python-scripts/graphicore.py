#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
running the fontforge scripts in the graphicore folder on fontforge startup

@author: Lasse Fister
@organization: http://graphicore.de
@copyright: Copyright (c) 2011, Lasse Fister lasse@graphicore.de, http://graphicore.de
@license: BSD

All rights reserved. This program and the accompanying materials are made
available under the terms of the BSD which accompanies this distribution, and
is available at http://www.opensource.org/licenses/bsd-license.php
"""

import sys
import os

#putting the folder of this file on the list of import paths
sys.path.reverse();
sys.path.append(os.path.dirname(sys.modules[__name__].__file__))
sys.path.reverse();

#load the modules
import graphicore.shell