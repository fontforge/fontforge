/* Copyright (C) 2000-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FONTFORGE_COLORP_H
#define FONTFORGE_COLORP_H

    /* how to convert rgb to grey scale */
#define LRD	3
#define LGN	6
#define LBL	2	/* Should be 1 */

/* Convert an RGB value [0,255]*3 into a grey value [0,255] */
# if LRD+LGN+LBL!=11
#  define RGB2GREY(r,g,b)	((LRD*(r)+LGN*(g)+LBL*(b))/(LRD+LGN+LBL))
# else
#  define RGB2GREY(r,g,b)	( ( (LRD*(r)+LGN*(g)+LBL*(b)) *2979)>>15 )
# endif

#define COLOR2GREY(col)	(RGB2GREY(COLOR_RED(col),COLOR_GREEN(col),COLOR_BLUE(col)))
/* convert an RGB value into a grey fraction [0.0,1.0] */
#define COLOR2GREYR(col) ((LRD*COLOR_RED(col)+LGN*COLOR_GREEN(col)+LBL*COLOR_BLUE(col))/((LRD+LGN+LBL)*255.0))

/* for a mono screen, should a color be white, or black? */
#define COLOR2WHITE(col)	(COLOR_RED(col)*LRD+COLOR_GREEN(col)*LGN+COLOR_BLUE(col)*LBL>=(LRD+LGN+LBL)*128)
#define COLOR2BLACK(col)	(COLOR_RED(col)*LRD+COLOR_GREEN(col)*LGN+COLOR_BLUE(col)*LBL< (LRD+LGN+LBL)*128)

#endif /* FONTFORGE_COLORP_H */
