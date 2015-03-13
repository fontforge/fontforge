#!/usr/bin/python

import SimpleHTTPServer
import SocketServer
import logging
import cgi
import subprocess
import json
import urllib
import re

PORT = 8000 # this must match up to the port used by the following files:
            # osx/FontForge.app/Contents/Resources/breakpad/localhost-8000.webloc
            # osx/FontForge.app/Contents/Info.plist.in
FFBP_DIR = "/Applications/FontForge.app/Contents/Resources/breakpad/"
cmd_minidump_stackwalk = FFBP_DIR + "/minidump_stackwalk"
symbolPath             = FFBP_DIR + "/symbols/x86_64/"
dumpFilePath = "/tmp/fontforge.dmp"

class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def do_GET(self):
        logging.error(self.headers)
	self.send_response(200)
        self.send_header('Content-type','text/html')
        self.end_headers()

        bt = ""
        with open( "/tmp/fontforge.dmp.backtrace.4", 'r') as f:
            bt = f.read()
        f.closed

        self.wfile.write("<html><body>")
        self.wfile.write("<p>Please report this crash so we can try to stop it happening for everybody.")
        self.wfile.write("<p>If you are a github user, clicking the below link will let you easily submit the backtrace to github and then you can keep an eye on the issue in case the developers need some more information to fix it.<br/>")
        self.wfile.write('<p>Report with <a href="https://github.com/fontforge/fontforge/issues/new?title=FontForge%20Breakpad%20Report&body=' + bt + '">github</a>')

        self.wfile.write("</body></html>")
        #SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

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
        # Open a browser at ourselves
        # 
        subprocess.call(["open", FFBP_DIR + "/localhost-8000.webloc" ])


        for item in form.list:
            logging.error(item)
        SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

Handler = ServerHandler

httpd = SocketServer.TCPServer(("127.0.0.1", PORT), Handler)

print "serving at port", PORT
httpd.serve_forever()

