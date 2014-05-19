"""
    Copyright (C) 2013 Mayank Jha <mayank25080562@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
"""
    Copyright (C) 2013 Mayank Jha <mayank25080562@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
import os
from fc.BitmapHandler import BitmapCompare
import shutil
import pkg_resources

class DocCompare:
    def basicCompare(self,testpath, mockfont, fontsize):
        docpath = pkg_resources.resource_filename("fc","docs/doc.txt")
        bashcommand = "hb-view --output-format=\"png\" --output-file=\"/var/tmp/test.png\" --font-size="+str(fontsize)+" --text-file=\""
        bashcommand+=docpath+"\" "+"\""+testpath+"\""
        os.system(str(bashcommand))
        print bashcommand
        thefile = pkg_resources.resource_filename("fc",mockfont.highresdocfile)
        shutil.copy(thefile,"/var/tmp/standard.png")
        cm = BitmapCompare()
        score = cm.basicCompare("/var/tmp/test.png",\
        "/var/tmp/standard.png",1000,0,True)
