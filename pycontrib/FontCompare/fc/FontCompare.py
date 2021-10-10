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
"""
Contains methods for making comparisons using a host of tests.
They are also for modifying a glyph by altering its
serif, stroke , stem thickness, size, italic angle etc.
and then later producing scores by doing bitmap comparison
"""
from fc.BitmapHandler import BitmapCompare
from fc.mockify import MockFont
import shutil
import pkg_resources
thefile = pkg_resources.resource_filename("fc","data/masterspritebold.bmp")
shutil.copy(thefile,"/var/tmp/tmpb.bmp")
thefile = pkg_resources.resource_filename("fc","data/masterspritenormal.bmp")
shutil.copy(thefile,"/var/tmp/tmpn.bmp")
thefile = pkg_resources.resource_filename("fc","data/masterspriteitalic.bmp")
shutil.copy(thefile,"/var/tmp/tmpi.bmp")
class FontCompare(object):

    def font_basiccompare(self, Testfont, mockfont):
        final=list()
        
        mx=max(Testfont.ascent,mockfont.ascent)
        mn=min(Testfont.ascent,mockfont.ascent)
        score1 = int((1/float(abs(mx-mn)))*10) if (mx-mn)!=0 else 10
        final.append(("Ascent Score: ",score1))
        
        mx=max(Testfont.descent,mockfont.descent)
        mn=min(Testfont.descent,mockfont.descent)
        score2 = int((1/float(abs(mx-mn)))*10) if (mx-mn)!=0 else 10
        final.append(("Descent Score: ",score2))
        
        mx=max(Testfont.capHeight,mockfont.capHeight)
        mn=min(Testfont.capHeight,mockfont.capHeight)
        score3 = int((1/float(abs(mx-mn)))*10) if (mx-mn)!=0 else 10
        final.append(("Cap Height: ",score3))
        
        mx=max(Testfont.strokewidth,mockfont.strokewidth)
        mn=min(Testfont.strokewidth,mockfont.strokewidth)
        score4 = int((1/float(abs(mx-mn)))*10) if (mx-mn)!=0 else 10
        final.append(("Stroke Width Score: ",score4))
        
        mx=max(Testfont.upos,mockfont.upos)
        mn=min(Testfont.upos,mockfont.upos)
        score5 = int((1/float(abs(mx-mn)))*10) if (mx-mn)!=0 else 10
        final.append(("Underline Position Score: ",score5))
        
        mx=max(Testfont.uwidth,mockfont.uwidth)
        mn=min(Testfont.uwidth,mockfont.uwidth)
        score6 = int((1/float(abs(mx-mn)))*10) if (mx-mn)!=0 else 10
        final.append(("Underline Width Score: ",score6))
        
        mx=max(Testfont.xHeight,mockfont.xHeight)
        mn=min(Testfont.xHeight,mockfont.xHeight)
        score7 = int((1/float(abs(mx-mn)))*10) if (mx-mn)!=0 else 10
        final.append(("x Height Score: ",score7))
        
        test=Testfont.weight
        standard=mockfont.weight
        score8 = 10 if test==standard else 0
        final.append(("PostScript weight string",score8))
        
        score=score1+score2+score3+score4+score5+score6+score7+score8;
        final.append(("Average Basic Score: ",score/8.0))
        Testfont.close()
        return final

    def font_facecompare(self, Testfont, mockfont, glyphRange, \
    resolution, ptsize, pixeldepth, fonttype):
        spritepath = "/var/tmp/tmpn.bmp"
        if fonttype is "bold":
            spritepath = "/var/tmp/tmpb.bmp"
            for i in range (glyphRange[0],glyphRange[1]):
                if i in Testfont:
                    Testfont[i].changeWeight(50,"auto",0,0,"auto")
        if fonttype is "italic":
            Testfont.selection.all()
            Testfont = Testfont.italicize(-13)
            spritepath = "/var/tmp/tmpi.bmp"
        scores = list()
        comparator = BitmapCompare()
        pixelsize = (resolution*ptsize)/72
        print spritepath
        for i in range (glyphRange[0],glyphRange[1]):
            if i in Testfont:
                Testfont[i].export("/var/tmp/tmp.bmp",pixelsize,1)
                Testfont[i].export("tmp.bmp",pixelsize,1)
                glyphscore = comparator.basicCompare("/var/tmp/tmp.bmp",\
                spritepath,pixelsize,(i-glyphRange[0])*pixelsize)
                glyphscore*=100
            else:
                glyphscore=0
            scores.append((i,round(glyphscore)))
        Testfont.close()
        return scores
