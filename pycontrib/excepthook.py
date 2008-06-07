# Contributed by Max Rabkin, 2008
# With this installed python exceptions will be reported through the
# fontforge user interface and not to stderr.

import sys
import traceback
import fontforge

def excepthook (*args):
    tb = ''.join(traceback.format_exception(*args))
    message = ''.join(traceback.format_exception_only(*args[:2])).strip()

    fontforge.logWarning(tb.replace('%', '%%'))
    fontforge.postError('Unhandled exception',
                        message.replace('%','%%'))

if fontforge.hasUserInterface():
    # only install the hook if this session has UI
    sys.excepthook = excepthook
