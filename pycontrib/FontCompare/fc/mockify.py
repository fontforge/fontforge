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
from fc.BitmapHandler import CreateSpriteSheet
import shelve
import fontforge
import os
class MockFont:
    ascent=0
    descent=0
    strokewidth=0
    uwidth=0
    upos=0
    xHeight=0
    glyphs = list()
    capHeight=0
    normalglyphfile = "data/masterspritenormal.bmp"
    boldglyphfile = "data/masterspritebold.bmp"
    italicglyphfile = "data/masterspriteitalic.bmp"
    highresdocfile = "data/highres.png"
    def __init__(self,fontpath,pixelsize,glyphrange):
        font = fontforge.open(fontpath)
        self.ascent = font.ascent
        self.descent = font.descent
        self.upos = font.upos
        self.uwidth = font.uwidth
        self.xHeight = font.xHeight
        self.capHeight = font.capHeight
        self.weight = font.weight
        #normal 
        CreateSpriteSheet(pixelsize,font,glyphrange,"normal")
        font = fontforge.open(fontpath)
        #bold
        CreateSpriteSheet(pixelsize,font,glyphrange,"bold")
        font  = fontforge.open(fontpath)
        #italic
        CreateSpriteSheet(pixelsize,font,glyphrange,"italic")
        #docfile,highres
        bashcommand = "hb-view --output-format=\"png\" --output-file=\"data/highres.png\" --font-size=160 --text-file=\""
        bashcommand+="docs/doc.txt\" \""+fontpath+"\""
        os.system(str(bashcommand))

def main():
    mock_font = shelve.open("data/mockfile.mcy")
    mock_font["font"] = MockFont("../unittests/lohit.ttf",100,(0x900,0x97f))
    mock_font.close()

if  __name__ =='__main__':main()
"""
explanation--> 

"../unittests/lohit.ttf" is the path where the font file is stored
so all you need to do is change the path of the font file, for making 
your own mockfonts.

"""
