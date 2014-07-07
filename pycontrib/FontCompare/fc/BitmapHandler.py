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
from PIL import Image
def white_bg_square(img):
    "return a white-background-color image having the img in exact center"
    size = (max(img.size),)*2
    layer = Image.new('1', size, 1)
    layer.paste(img, tuple(map(lambda x:(x[0]-x[1])/2, zip(size, img.size))))
    return layer

class BitmapCompare:
    def basicCompare(self,testbmp,standardsprite,size,xoffset,equal=False):
        master = Image.new('1',(size,size),0)
        img = Image.open(testbmp)
        square_one = white_bg_square(img)
        square_one.resize((size,size))
        master.paste(square_one,(0,0))
        master.save(testbmp)
        if equal:
            master = Image.new('1',(size,size),0)
            img = Image.open(standardsprite)
            square_one = white_bg_square(img)
            square_one.resize((size,size))
            master.paste(square_one,(0,0))
            master.save(standardsprite)
        sprite = Image.open(standardsprite) 
        test = Image.open(testbmp)
        count = 0
        i=0
        while i<size:
            j=0
            while j<size:
                if sprite.getpixel((j+xoffset,i)) == test.getpixel((j,i)):
                    count+=1
                j=j+1
            i=i+1
        return float(count)/(size*size)


class CreateSpriteSheet:
    def __init__(self,pixelsize,font,glyphrange,effects):
        master_width = (pixelsize * (glyphrange[1]-glyphrange[0]+1) )
        #seperate each image with lots of whitespace
        master_height = pixelsize
        oldfont = font
        print "the master image will by %d by %d" % (master_width, master_height)
        print "creating image..."
        master = Image.new(
        mode='1',
        size=(master_width, master_height),
        color=0) # fully transparent
        print "created."
        if effects == "italic":
            font.selection.all()
            font = font.italicize(-13)
        count=0
        for i in range (glyphrange[0],glyphrange[1]):
            location = pixelsize*count
            try:
                t=font[i]
                if effects == "bold":
                    font[i].changeWeight(50,"auto",0,0,"auto")
                font[i].export("temp.bmp",pixelsize,1)
                img = Image.open("temp.bmp")
                print "adding %s at %d..." % (str(i)+".bmp", location),
                square_one = white_bg_square(img)
                square_one.resize((pixelsize, pixelsize))
                master.paste(square_one,(location,0))
                print "added."
            except:
                print "ooopsy"
            count+=1
        print "done adding pics."
        print "saving mastersprite.bmp..."
        master.save('data/mastersprite'+effects+'.bmp' )
        print "saved!"
        font.close()
