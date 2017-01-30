#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
FontForge Interactive Python Shell
Modify font-data in the current FontForge session live with python scripting.

@author: Lasse Fister
@organization: http://graphicore.de
@copyright: Copyright (c) 2011 Lasse Fister
@license: BSD

All rights reserved. This program and the accompanying materials are made
available under the terms of the BSD which accompanies this distribution, and
is available at http://www.opensource.org/licenses/bsd-license.php

All the kudos go to:
    http://ipython.scipy.org
    http://live.gnome.org/Accerciser
    http://ipython.scipy.org/moin/Cookbook/EmbeddingInGTK
I only packed their code in the FontForge context as a quick and dirty way of having a shell.

"""

import sys
import os
import gtk
import fontforge;
import pango
import platform

if len(sys.argv) == 0:
    # some versions of IPython need content in sys.argv
    sys.argv.append('')

from ipython_view import *
import gdraw

def runShell(data = None, glyphOrFont = None):
    """Run an ipython shell in a gtk-widget with the current fontforge module in the namespace"""
    FONT = "monospace 10"
    W = gtk.Window()
    W.set_resizable(True)
    W.set_size_request(750,550)
    W.set_title("FontForge Interactive Python Shell")
    S = gtk.ScrolledWindow()
    S.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
    V = IPythonView()
    V.updateNamespace({'fontforge': fontforge}) #important
    V.modify_font(pango.FontDescription(FONT))
    V.set_wrap_mode(gtk.WRAP_CHAR)
    S.add(V)
    W.add(S)
    W.show_all()
    W.connect('delete_event',lambda x,y:False)
#    W.connect('destroy',lambda x:gtk.main_quit())

    # Start gtk loop here!
    gdraw.gtkrunner.sniffwindow(W)
    gdraw.gtkrunner.start()
if fontforge.hasUserInterface():
    fontforge.registerMenuItem(runShell, None, None, ("Font","Glyph"),
                                None, "Interactive Python Shell");
