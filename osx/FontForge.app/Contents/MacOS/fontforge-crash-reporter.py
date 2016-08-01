#!/usr/bin/python

import SimpleHTTPServer
import SocketServer
import logging
import cgi
import subprocess
import json
import urllib
import re
import sys
import time, threading

PORT = 8000 # this must match up to the port used by the following files:
            # osx/FontForge.app/Contents/Resources/breakpad/localhost-8000.webloc
            # osx/FontForge.app/Contents/Info.plist.in
FFBP_DIR = "/Applications/FontForge.app/Contents/Resources/breakpad/"
cmd_minidump_stackwalk = FFBP_DIR + "/minidump_stackwalk"
symbolPath             = FFBP_DIR + "/symbols/x86_64/"
dumpFilePath = "/tmp/fontforge.dmp"


# let the timeout handler close the httpd server
httpd = None

def getNumberOfRunningFontForges():
    stdout = subprocess.check_output(['ps', '-eo' ,'pid,args'])
    count = 0
    for line in stdout.splitlines():
        pid, cmdline = line.split(' ', 1)
        if cmdline.startswith('/Applications/FontForge.app/Contents/Resources/opt/local/bin/fontforge'):
            count += 1
    return(count)


#
# if there is no fontforge process around for 
# 2 minutes then we should probably call it a day too
#
shouldWeDiePreviousCount = 1    
def shouldWeDie():
    global shouldWeDiePreviousCount
    global httpd
    print(time.ctime())
    count = getNumberOfRunningFontForges()
    print(count) 
    if shouldWeDiePreviousCount==0 and count==0:
        if httpd is not None:
            httpd.shutdown()
        sys.exit()
    shouldWeDiePreviousCount = count
    threading.Timer(60, shouldWeDie).start()

class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def fileToString(self, filename):
        ret = ""
        with open( filename, 'r') as f:
            ret = f.read()
        f.closed
        return ret
        

    def do_GET(self):
        logging.error(self.headers)


        # allow css, js, and png files to get out to make the web interface
        # more appealing.
        logging.error(self.path)

        if( self.path == "/support/bootstrap.min.css"
        or self.path == "/support/bootstrap.min.js"
        or self.path == "/support/jquery-2.1.3.min.js"
        or re.match( '^/[-a-zA-Z. _]+$', self.path ) is not None ):
             ct = "image/png"
             if self.path.endswith(".css"):
                 ct = "text/css"
             if self.path.endswith("svg"):
                 ct = "image/svg+xml"
             if self.path.endswith(".js"):
                 ct = "text/javascript"
	     self.send_response(200)
             self.send_header('Content-type', ct)
             self.end_headers()
             data = self.fileToString( FFBP_DIR + "/webinterface/" + self.path )
             self.wfile.write( data )
             return

        #
        # those nasty hackers might be trying to trick us 
        #
        if self.path != "/":
             logging.error("HACK ATTEMPT FOR URL: " + self.path)
	     self.send_response(200)
             self.send_header('Content-type','text/html')
             self.end_headers()
             self.wfile.write("nice try")
             return
            
        #
        # OK, just the base index.html file then, load and merge.
        #
	self.send_response(200)
        self.send_header('Content-type','text/html')
        self.end_headers()
        bt   = self.fileToString( "/tmp/fontforge.dmp.backtrace.4" )
        data = self.fileToString( FFBP_DIR + "/webinterface/index.html" )
        data = re.sub( '{{report}}', bt, data )
        data = re.sub( '{{report_raw}}', self.fileToString( "/tmp/fontforge.dmp.backtrace" ), data )
        data = re.sub( '{{title}}',  'FontForge%20Breakpad%20Crash%20Report', data )
        data = re.sub( '{{title_raw}}',  'FontForge Breakpad Crash Report', data )
        self.wfile.write( data )


    # This gets the raw minidump from the osx crash reporter
    # we then create a backtrace using the local breakpad symbols
    # in preparation for uploading that someplace. After this we 
    # launch a web browser with our location. The above GET handler
    # will show the user a web page with links to upload this data 
    # to various places.
    def do_POST(self):
        host = self.headers.get('Host')
        logging.error("host" + host);

        logging.error(self.headers)
        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={'REQUEST_METHOD':'POST',
                     'CONTENT_TYPE':self.headers['Content-Type'],
                     })

        if "FontForgeGitHash" not in form or "upload_file_minidump" not in form:
            logging.error("POST is missing data!");
            print "<H1>Error</H1>"
            print "Please fill in the name and addr fields."
            return

        logging.error("hash: " + form["FontForgeGitHash"].value );
        metadata =  dict([])
        for k in ["FontForgeGitHash", "ver", "FontForgeBuildTime", "ptime", "guid"]:
            metadata[k] = form[k].value
        metadata["user-agent"] = self.headers.get("user-agent")


        with open( dumpFilePath, 'w') as f:
            f.write(form["upload_file_minidump"].value)
        f.closed
        bt = subprocess.check_output([cmd_minidump_stackwalk, dumpFilePath, symbolPath ])
        with open( dumpFilePath + ".backtrace", 'w') as f:
            json.dump( metadata, f, indent=4 )
            f.write('\n\n')
            f.write('-----------------------------------------')
            f.write('\n\n')
            f.write(bt)
        f.closed

        with open( dumpFilePath + ".backtrace", 'r') as fin:
            with open( dumpFilePath + ".backtrace.4", 'w') as fout:
                 data = fin.read()
                 data = data[:4000] + "E"
                 data = re.sub( '$', '%0A',    data, 0, re.MULTILINE )
                 data = re.sub( '"', '&quot;', data, 0, re.MULTILINE )
                 fout.write( '```%0A\n' + data + '\n```%0A' )
            fout.close
        fin.close


        with open( dumpFilePath + ".backtrace", 'r') as fin:
            with open( dumpFilePath + ".backtrace.enc", 'w') as fout:
                 data = fin.read()
                 data = urllib.quote_plus( data )
                 fout.write( '<?xml version="1.0" encoding="UTF-8"?>\n' )
                 fout.write( '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n')
                 fout.write( '<plist version="1.0">\n' )
                 fout.write('<dict>\n')
                 fout.write('<key>URL</key>\n')
                 fout.write('<string>')
                 fout.write('https://github.com/fontforge/fontforge/issues/new?title=fontforge-crash&amp;body=')
                 fout.write( data )
                 fout.write('</string>\n')
                 fout.write('</dict>\n')
                 fout.write('</plist>\n')
            fout.close
        fin.close

        #
        # Open a browser at ourselves to activate the above GET method
        # 
        subprocess.call(["open", FFBP_DIR + "/localhost-8000.webloc" ])


        for item in form.list:
            logging.error(item)
        SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

Handler = ServerHandler

httpd = SocketServer.TCPServer(("127.0.0.1", PORT), Handler)

threading.Timer(60, shouldWeDie).start()
print "serving at port", PORT
httpd.serve_forever()

