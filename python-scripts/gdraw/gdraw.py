#!/usr/bin/python
'''ctypes wrapper to Attach the GDraw event handler to the gtk main loop.

Copyright <hashao2@gmail.com> 2009

#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation; either version 3 of the License, or
#       (at your option) any later version.
#       
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#       
#       You should have received a copy of the GNU General Public License
#       along with this program; if not, write to the Free Software
#       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#       MA 02110-1301, USA.

We use a gdraw timeout handler to call the gtk.main_iteration() periodically.

You might also want to copy setup_syspath() in gdraw.py to setup proper
sys.path for fontforge scripts.

Usage: 
    from gdraw import gtkrunner

    w = gtk.Window()
    b = gtk.Button('click')
    w.add(b)
    w.show_all()

    gtkrunner.start()

Example:
    t = Timer()
    tid = t.add(timeout, func[, data])
    t.remove(t)

    # top start/stop gtk main iteration
    g = GtkRunner()
    g.start()
    g.stop()
'''

import fontforge

GTIMEOUT = 2000 # milliseconds interval to process GDraw events.


# You should copy function setup_syspath() to your ff script and call it before
# import gdraw. The gdraw.py should be put in ~/.FontForge/python/modules or
# equivalent.
def setup_syspath(modpath="modules"):
    '''Setup module path for local modules of Fontforge scripts.  Put local
    modules under 'modules' subdirectory: ~/.FontForge/python/modules

    @modpath: a path under ~/.FontForge/python, default to "modules".
    '''
    import sys
    import os
    x = sys._getframe().f_code.co_filename
    modpath = os.path.join(os.path.dirname(x), modpath)
    if modpath not in sys.path:
        sys.path.append(modpath)

__all__ = ['Timer', 'GtkRunner', 'gtkrunner']

from _gdraw import *
from ctypes import *
import types

# Callback object to save callback informations
CallBackFunc = CFUNCTYPE(c_int, py_object)
class CallInfo(Structure):
    _fields_ = [
            ('func', CallBackFunc),
            ('data', py_object),
            ]

import gtk

class Timer:
    '''A Timer class for everyone else to add timer handler to 
    gdraw's event loop.
    '''
    def __init__(self):
        self.timer_map = {} # Keep a ref to the callback to avoid gc.
        self.setup_event_window()

    def setup_event_window(self):
        '''Setup a gdraw hidden event window to handle timeout events.'''

        # Need to save a reference to prevent garbage collector.
        self.EHFUNC = EHFUNC = CFUNCTYPE(c_int, GWindow, POINTER(GEvent))
        self.eventhandler = eventhandler = EHFUNC(self._event_handler)

        # Create a hidden window and wait for timer to fire up
        wattrs = GWindowAttrs()
        wattrs.mask=wam_events|wam_nodecor
        wattrs.event_masks = 1<<et_timer
        wattrs.nodecor = 1

        pos = GRect()
        pos.x = pos.y = 0
        pos.width = pos.height = 1

        self.win = GDrawCreateTopWindow(None, pos, eventhandler, None, wattrs)
        GDrawSetVisible(self.win, 0)

        return
        def dodo(*args):
            print 'aaa'
            return False
        self.add(1000, dodo)

    def __del__(self):
        '''Cleanup everything.'''
        # Hopefully it's get called.
        GDrawDestroyWindow(self.win)

    def _event_handler(self, gw, event):
        evt = event.contents
        print "_event_handler()"
        if evt.type == et_timer:
            timer = evt.u.timer.timer
            tkey = addressof(timer.contents)

            if tkey in self.timer_map:
                pinfo = evt.u.timer.userdata
                ci = CallInfo.from_address(pinfo)
                func = ci.func
                data = ci.data
                ret = func(data)
                if not ret:
                    self.remove(timer)
        return 1

    def add(self, timeout, func, data=None):
        '''
        timeout: in millisecond.
        frequency: 0 => one shot.
        func: stop if return false
        '''
        ci = CallInfo()
        ci.func = CallBackFunc(func)
        ci.data = data

        print "timer.add timeout", timeout
        frequency = 1 # Use return value of func() to decide repeat like gtk.
        timer = GDrawRequestTimer(self.win, timeout, timeout, byref(ci))

        tkey = addressof(timer.contents)
        self.timer_map[tkey] = (ci, timer)
        return timer

    def remove(self, timer):
        '''Remove the give timer.'''
        if self.is_active(timer):
            GDrawCancelTimer(timer)
            tkey = addressof(timer.contents)
            del self.timer_map[tkey]

    def clear(self):
        '''Remove all the timers.'''
        if not self.timer_map:
            return

        for k in self.timer_map:
            timer = self.timer_map[k][1]
            GDrawCancelTimer(timer)
        self.timer_map = {}

    def is_active(self, gtimer):
        '''Test if a given GTimer* is active.'''
        if isinstance(gtimer, POINTER(GTimer)):
            tkey = addressof(gtimer.contents)
        elif isinstance(gtimer, GTimer):
            tkey = addressof(gtimer)
        else:
            return False
        return tkey in self.timer_map

class GtkRunner:
    '''A Helper class to start and stop gtk main iteration.'''
    def __init__(self, timer=None):
        self.gtk_timer = None # a GTimer instance for gtk.
        self.timer = timer # Timer instance

    def _do_main(self, *args):
        '''The function called by the gdraw timeout handler.'''
        print "do_main"
        while gtk.events_pending():
            gtk.main_iteration(False)
        return True

    def stop(self):
        '''Stop gtk event handler.'''
        if not self.timer:
            self.timer = Timer()

        timer = self.timer
        if (self.gtk_timer is not None):
            self._do_main()
            timer.remove(self.gtk_timer)
            self.gtk_timer = None

    def OnDestroyWindow(self, widget, fd ):
        print fd
        fontforge.removeGtkWindowToMainEventLoopByFD( fd )
        self.stop()
        return True

    def sniffwindow(self,w):
        '''sniff key presses for a gtk window'''
        print "sniffwindow w", w
        print "sniff active font:", fontforge.activeFont()
        w.connect("key-release-event", self._do_main)
        fontforge.addGtkWindowToMainEventLoop(w.window.xid)
        fd = fontforge.getGtkWindowMainEventLoopFD(w.window.xid)
        w.connect('destroy', self.OnDestroyWindow, fd )

    def sniffwindowid(self,xid):
        '''sniff key presses for a gtk window'''
        print "sniffwindowid xid", xid
        #w.connect("key-release-event", self._do_main)

    def start(self, timeout=GTIMEOUT):
        '''Process gtk events every some time.'''
        if not self.timer:
            self.timer = Timer()

        timer = self.timer
        gtk_timer = self.gtk_timer
        if (gtk_timer is not None):
            if timer.is_active(gtk_timer):
                return

        # handle possible gtk events first.
        # print 'gtk_timer'
        self._do_main()
        self.gtk_timer = timer.add(timeout, self._do_main)

gtkrunner = GtkRunner()
# vim:ts=8:sw=4:expandtab:encoding=utf-8
