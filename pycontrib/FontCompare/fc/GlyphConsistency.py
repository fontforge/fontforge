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
from fc.BitmapHandler import BitmapCompare
bitcomp = BitmapCompare()
class GlyphConsistency:
    #computes the number of selfIntersects in the glyph is any across
    #all layers, it is a self consistency check
    def glyph_basicConsistency(self, font, glyphrange):
        scores = list()
        total = 0
        for i in range (glyphrange[0],glyphrange[1]):
            #worth outputting
            try:
                t=font[i]
		if i in font:
		    if font[i].isWorthOutputting():
			scoreFactor = 1
		    else:
			scoreFactor = 0
		    score=10
		    if font[i].layers[1].selfIntersects():
			score=0
		    else:
		        score = score * scoreFactor
            except:
                score = 0
            #no counter intersection
            total+=score
            scores.append((i,score))
        #scores.append(("Basic Consistency Score: ",total/len(scores)))
        return scores

    def glyph_basicset_consistency(self, font, range_tuple):
        total = 0
        #bbox consistency
        xmin_cords = list()
        ymin_cords = list()
        ymax_cords = list() 
        xmax_cords = list()
        #bearing consistency 
        lbearing = list()
        rbearing = list()
        #layer count consistency
        layer_cnts = list()
        for i in range (range_tuple[0],range_tuple[1]):
            if i in font:
                if not font[i].boundingBox()[0] in xmin_cords:
                    xmin_cords.append(font[i].boundingBox()[0])
                if not font[i].boundingBox()[1] in ymin_cords:
                    ymin_cords.append(font[i].boundingBox()[1])
                if not font[i].boundingBox()[2] in xmax_cords:
                    xmax_cords.append(font[i].boundingBox()[2])
                if not font[i].boundingBox()[3] in ymax_cords:
                    ymax_cords.append(font[i].boundingBox()[3])
                if not font[i].left_side_bearing in lbearing:
                    rbearing.append(font[i].left_side_bearing)
                if not font[i].right_side_bearing in rbearing:
                    rbearing.append(font[i].right_side_bearing)
                if not font[i].layer_cnt in layer_cnts:
                    layer_cnts.append(font[i].layer_cnt)
            total+=1
        count=len(xmin_cords)+len(xmax_cords)+len(ymin_cords)+len(ymax_cords)
        count = count+ len(rbearing) + len(lbearing) + len(layer_cnts)
        set_basic_score = count/float(total*7)
        return set_basic_score*10

    def glyph_round_consistency(self, font, glyphrange, pixelsize):
        #script and rounding consistency
        set_round_score=0.0
        total=0
        net_score = list()
        for i in range (glyphrange[0],glyphrange[1]):
            if i in font:
                score =  self.glyph_round_compare(font[i],pixelsize)
                print score
                set_round_score+=score
                total+=1
        font.close()
        return (set_round_score/float(total))*10

    def glyph_round_compare(self,glyph,pixelsize):
        glyph.export("/var/tmp/before.bmp",pixelsize,1)
        glyph.round()
        glyph.export("/var/tmp/after.bmp",pixelsize,1)
        score =(bitcomp.basicCompare("/var/tmp/before.bmp", \
        "/var/tmp/after.bmp",pixelsize,0,True))
        if score == 1:
            return 1
        else:
            return 0
