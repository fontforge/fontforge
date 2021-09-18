What on Earth Possessed me...
=============================

.. image:: /images/fftype300.svg
   :align: right
   :width: 200px

.. epigraph::

  | **MALVOLIO**
  |
  | By my life, this is my lady's hand, these be her
  | very C's, her U's and her T's and thus she makes her
  | great P's. It is, in contempt of question, her hand.
  |
  | **Sir ANDREW**
  | Her C's, her U's and her T's: why that?

  -- Twelfth Night, II, v
  -- Shakespeare

I have always been interested in calligraphy, especially the eccentricities of
the swash and black letter capitals.

My father was deeply concerned with Renaissance printing, being a textual
bibliographer of Shakespeare. His major focus was in the errors printers were
likely to make, nevertheless I grew up in an environment where the long-s and
other archaic devices appeared. My father used to brag that a great aunt of his
still used the long-s in her hand-written notes.

Why that indeed. Some questions may have no answers.

In the early '80s I was working at JPL and I met my first bitmap display. The
primary use of this display was to make movies of a simulated fly-by of Olympus
Mons (on Mars). I used it to design my first bitmap font.

In the late '80s I bought my first computer, a Mac II. And with it a bought a
little program called Fontastic which allowed people to design their own 'FONT'
resources (mac bitmap fonts).

Then a friend, Godfrey DiGiorgi, recommended that I buy a copy of Fontographer
and design PostScript fonts. I was leery initially. How could a rasterizer match
the quality of a hand crafted bitmap? But eventually I succumbed to the
attractions of the cubic spline.

In the meantime I had studied calligraphy, and once I had Fontographer began
designing fonts based on various (latin) calligraphic hands.

In the early '90s I was working at a little web start-up company, called
NaviSoft, which was almost immediately bought by AOL. My product was an
html-editor (best known as AOLpress). As I was working to convert it to handle
Unicode I became concerned about the lack of Unicode fonts. I began working on
my own Unicode font (just the alphabetics and symbols, I knew there was no way
I'd be able to deal with all the CJK characters). I designed a font based on
Caslon's work with Bold and Italic variants. And then I started working on
monospaced and sans-serif families (I called the sans-serif design "Caliban" as
a play on Arial).

Aldus (the makers of Fontographer) had been bought by MacroMedia, and MacroMedia
seemed to have no interest in continuing Fontographer. So development on
Fontographer ceased. It did not support OpenType, and its unicode support was
minimal. I began to write little programs to decode Type1 fonts and fix them up
in various ways.

AOL did not really know what to do with AOLpress. AOLpress had been designed
with web designers in mind, not with Steve Case's mother (which was AOL's target
audience). So development on AOLpress ceased and the Unicode/CSS version never
was completed. I continued to work on my fonts however and continued to be
dissatisfied with Fontographer. In 1998 my AOL options matured and I was able to
retire.

I wanted to try to become a primatologist and had made arrangements to spend 4
months in Madagascar as a field assistant to Chia Tan studying the Greater
Bamboo lemur (\ *Hapalemur simus*). Sadly I found that I was not really cut out
for that life. I had a hard time recognizing individual animals, and found that
after a few months the leaches were more annoying than I had expected.

So I gave up on that.

Instead I set about working on an improved version of my html editor, and
started by writing my own Unicode based widget set for it (this was before pango
was out). When the widget set was usable I decided to write a small application
to test it, and something to display the splines of a postscript font seemed
just the thing. Having done that I figured I might as well allow people to edit
those splines and save it back. And so was born the first version of PfaEdit.

Somehow the html-editor never got written.

I quickly discovered I was better at designing a font editor than I was at
designing fonts, so I gave up on them too.

After a couple months of work I had something which worked, or so I thought, and
I posted it to the web (my friend, Dan Kenan, very kindly gave me some space on
his server, aptly named bibliofile) on 7 November of 2000. Within a month I had
received my first bug report, and presumably had my first user.

I continued working on PfaEdit, adding support for pfb fonts and then in
December for truetype and bdf fonts. I learned about sourceforge and moved
PfaEdit there in April of 2001.

In April of 2001 I added support for type2 fonts embedded in an sfnt wrapper
(opentype fonts, but not the advanced typographic tables). In July of 2001
MinGyoon (from Korea) asked me if PfaEdit could support CID keyed fonts so I
learned about those and added support for them in August.

Valek Filippov suggested that I make PfaEdit be internationalizable, so I
provided a mechanism and he provided a Russian translation of the user interface
in June of 2001. On December of 2005 I gave up on my own system and switched to
GNU gettext.

.. list-table::
   :header-rows: 1
   :stub-columns: 1

   * - Language
     - Translator
     - Initial version
   * - English
     -
     -
   * - Russian
     - Valek Filippov
     - June 2001
   * - Japanese
     - Kanou Hiroki
     - August 2002
   * - French
     - Pierre Hanser
     - September 2002
   * - Italian
     - Claudio Beccari
     - February 2003
   * - Spanish
     - Walter Echarri
     - October 2004
   * - Vietnamese
     - Clytie Siddall
     - July 2006
   * - Greek
     - Apostolos Syropoulos
     - August 2006
   * - :small:`Simplified` Chinese
     - Lee Chenhwa
     - October 2006
   * - German
     - Philipp Poll
     - October 2006
   * - Polish
     - Michal Nowakowski
     - October 2006
   * - :small:`Traditional` Chinese
     - Wei-Lun Chao
     - August 2007

I started working on support for simple aspects of the OpenType GSUB and GPOS
tables in April of 2001 and finished the process (ignoring bugs of course) with
the contextual chaining lookups in August of 2003. Similarly I started adding
support for the various equivalent Apple Advanced Typography tables (primarily
'morx') at about the same time.

In an early attempt to get PfaEdit to generate instructions to grid-fit truetype
fonts, I set about to write a truetype instruction simulator so that I could
debug the generated code. It didn't work very well on real fonts. Then, in early
2001, I discovered `freetype <http://freetype.sf.net/>`_ and found that freetype
already did this (and did it right). At first I examined their code to try and
figure out what was wrong with mine, but eventually I gave that up and simply
used freetype as an instruction simulator. As things got more complicated (with
David Turner's permission, and many suggestions from Werner LEMBERG), I
eventually wrote a visual front end for freetype's built-in debugger. For a
while this lived in a separate program called mensis, but in March of 2003 I
integrated it into PfaEdit.

Many people urged me to provide a scripting interface to PfaEdit. At first I
could not understand the point -- font design needs a graphical interface after
all. But I was only looking at a small fraction of the tasks that could
potentially be done with such an interface, and in January of 2002 PfaEdit
gained the ability to run scripts.

In 2003 Yannis Haralambous invited me to talk at EuroTex. I fear I rather
disappointed him in my choice of subject matter -- I tried to do better the next
year when Apostolos Syropoulos invited me to EuroTex 2004 (but I overreached
myself then and made some incorrect assumptions). These conferences were the
first time I had actually met any of my users and were quite stimulating,
leading to many suggestions and requests. I learned about SVG fonts at EuroTex
2003 and implemented them soon thereafter.

Yannis was also working on a book, *Fontes & codages* in which FontForge
figures. He spent a lot of time making suggestions and finding bugs. He
encouraged me to support multi-master fonts and by February of 2004 I had done
so. Then I started working on Apple's distortable font technology (which has
many similarities to Adobe's multi-master, but is rather badly documented) and,
with help from Apple, had them working in April of 2004. I then extended
freetype's support for multi-master fonts to support Apple's distortable fonts.

In early 2004 people complained that the name "PfaEdit" no longer reflected the
abilities of the program and requested that I change it. Various people
suggested names (including me), but the one I liked the best, FontForge, came
from David Turner of freetype. And in March of 2004 PfaEdit changed its name to
FontForge.

At about the same time I wanted to provide a somewhat more complete ability to
handle PostScript Type3 fonts (or SVG fonts). So I implemented a multi-layered
editing mode which provided a rather clumsy interface to some of the facilities
of vector graphics programs.

In 2005 a Korean company asked me to do something. We had some difficulty
communicating (I don't speak Korean), but eventually I figured out that they
wanted to be able to group glyphs together. Prior to this FontForge handled
encodings as an integral part of the font, which didn't seem right, and it made
implementing groups impossible. So I had to rewrite much of the internals of
FontForge to redo encodings before I could even start on groups. This took
longer than I had thought it would, and by the time I finished (in July of 2005)
the Koreans seemed to have lost interest. Ah well.

I got interested in pdf files in October of 2005, and gave FontForge's Print
command the ability to print to a pdf file. Then I thought it would be kind of
fun to be able to read a font out of a pdf file. I was a little worried about
implementing this because I know that most fonts stored in pdf files are
sub-sets, and only contain the glyphs actually used in the pdf file itself. I
was convinced that I'd get lots of bug reports from people complaining that
FontForge didn't read the entire font. Nevertheless my sense of fun overcame my
fear of silly bug reports and I implemented it.

And I did get bug reports complaining that FontForge did not read the fonts
correctly.

And I don't think I was able to convince some of the complainers that the fonts
were incomplete in the pdf file. Ah well.

The X11 folk want to move away from the bdf format, so they came up with their
own format (call opentype bitmap, with extension "otb") which was essentially an
sfnt wrapper around a series of bitmap strikes with no outline font. I
implemented that back in July of 2003. But then in July of 2005 they wanted to
preserve the BDF properties as well. So we worked out a new table (called 'BDF
') to contain the properties from all the strikes in the font. Now it should be
possible to make a round trip conversion of bdf->otb->bdf and not lose any
information.

Many people complained about FontForge's ability to edit quadratic splines. I
had no experience editing quadratic splines before I wrote my original version,
I just made it behave like the cubic spline editor (which seemed obvious). But
doing the obvious makes it hard to create a font that uses some of the
optimizations in the ttf file, and made instructing the font confusing. So
between January and February of 2006 FontForge's quadratic editing capabilities
underwent an evolutionary change as people complained and I tried to fix things.

I have a testsuite for fontforge. Obviously. Originally it was very simple: a
set of script files which did various actions. If FontForge didn't crash, then I
presumed it worked. That was about all I could test, and although that's
important, there are a few other things which might be examined. So I wrote a
command to compare two fonts and see if they were equivalent. Originally this
had been a separate command (called sfddiff), but if I integrated it into
FontForge I could increase the abilities of the tests I wrote.

FontForge produced some rather naive type1 and type2 fonts which did not make
good use of the PostScript concept of subroutines. In June of 2006 I did a
substantial rewrite of the type2 output code and decreased the size of my output
fonts considerably. My new comparison command was helpful in debugging.
Nonetheless I introduced a number of bugs. Which got fixed, of course. But it
made me leery of doing the same thing for type1 output. After all, Adobe doesn't
even produce type1 fonts any more, so surely I don't need to optimize them.
Michael Zedler said otherwise, and after great effort on his part induced me (in
October 2006) to make better use of subroutines in Type1 output also. No bugs
yet... (but it's still October of 2006).

All of FontForge's dialogs had a fixed layout. Which works fine if you've only
got one language to support, but which looks really ugly (and worse can be
totally illegible) when the dialog is translated into a different language and
labels suddenly become longer (or shorter) and spill over into the textfield
they identify. There has been a sudden burst of people willing to do
translations recently. This mattered. So I stole the concept of boxes from gtk
and implemented them in my widget set (in August of 2006), allowing a dialog to
do its own layout to match the size of the things in it.

The pace of change seems to have slowed recently (Oct 2006) as all of the large
tasks have either been done or proved insurmountable. As more people use the
program they find more bugs and I have less time to do development. In the last
few years there have also been large internal changes which (I hope) are
practically invisible to users and cosmetic changes which make the dialogs look
nicer and more comprehensible but which aren't functional.

--------------------------------------------------------------------------------

My interface to GSUB/GPOS was not well thought out. I stored things in FontForge
at the feature level, while OpenType wants things done at the lookup level. I
thought lookups added an unnecessary level of complexity and ignored them. But
people complained (they always do) that once a font had been read in to
FontForge and saved out again it wouldn't work any more. And that was because I
had lost the ordering imposed by the lookups. So in early 2007 I had to redo
much of the internals of fontforge as it related to OpenType. I also changed the
Metrics View so it would handle all OpenType lookup types (rather than just
kerning).

And people didn't like my scripting language. Why hadn't I used python? (Well
because I didn't know python and was lazy about learning more stuff that I
didn't think would be useful to me). Various people told me that they just
couldn't use FontForge because it didn't support python. So I added python
support. Then I discovered that my build machine has such an old version of
python that it doesn't provide libpython -- and I can't upgrade my machine any
more because all the distros require booting from CD now (and my machine can't).

In May of 2007 I went to the Libre Graphics Meeting in Montreal, and as I
listened to the Inkscape talk on how they handled plugins, I realized that I
could do that too. So I extended the python interface to support python plugins
and menu items. Dave Crossland, as is his wont, had many requests, and had me
update the old Display dialog to support all the OpenType lookups (just as I'd
done for the Metrics View) and then merge that into the Print dialog too. Dave
also felt that FontForge should be able to store a font directly on the Open
Font Library website. Well, they had no API for this, so I had to sit down and
figure out http all over again and see what bits of the user API I needed to
walk through.

In June I started working on Adobe's feature files (I could support them now
that I was handling lookups properly), and found to my shock that

#. The syntax as presented by Adobe wasn't complete (could not represent all of
   opentype)
#. Some of the syntax that was presented hadn't been implemented by Adobe yet
   and was marked "Subject to change"
#. There was no easy way to represent the "Everything else" class (class 0) of a
   class set without enumerating every glyph by hand (which could not be
   translated into a class 0.
#. There was no way to distinguish a contextual class based lookup from a
   contextual coverage-table based lookup.
#. ... on and on ...

I had assumed that feature files were a stable useful format and found to my
distaste that they were not. I implemented the bits that Adobe hadn't
implemented, and extended them a bit so I could represent more of OpenType (and
told Adobe what my extensions were, but was told they didn't like them). Grump.
Well I wanted something to store as much of OpenType as I could, and I wasn't
going to wait for Adobe to come up with something (which they still haven't).

Apostolos gave me the spec for the new 'MATH' table. But that spec had MicroSoft
Confidential printed all over it and I wasn't about to touch it. Apostolos got
annoyed at my ignoring it, so in July he had Sergey from MS send me a copy of
the spec that no longer said "Confidential" on every page. Then I implemented
the new 'MATH' table.

I'd never had a good Embolden function. I'd tried various approaches and none
worked well. This year I decided to try a very simple idea: Use expand stroke
and then squash the glyph together so it was the same height it had been before.
That basically worked. Still a few oddities, but basically functional.

In July Michal Nowakowski gave me a patch which vastly improved the truetype
auto instructor. I told him I'd only accept it if he would support it. After
some initial grumbling he did so -- and then proceeded to make it even better!
Then about a week later Alexej Kryukov said he wanted to make the autoinstructor
support diagonal stems, and the two of them started working together on this.

At the Libre Graphics Meeting Dave demoed Raph Levien's spiro splines and
encouraged me to integrate them into fontforge. But Raph released under GPL and
wasn't willing to change, and I released under BSD and wasn't willing to change.
I got permission from Raph to repackage his spiro routines into a small shared
library (libspiro) which could be released separately from FontForge but to
which FontForge could link. And we had Raph's spiros in FontForge.

I realized that no only could I stick python into fontforge, but if I did a
little more work, I could stick fontforge into python. So I wrapped up most of
fontforge into a shared library that python could load. Dave Crossland had been
complaining (again) about the FontForge widget set. When was I going to move to
gtk? (well, I'd tried gtk back in 2004, and found it hard to use, and bits of it
ugly -- and less functional than my own widget set in the ways that mattered to
me, so I had given up on it). Dave offered to fund development of a gtk
fontforge UI, but only if I'd switch to GPL. I dislike GPL, it seems so
restrictive to me, so I said I wouldn't. Then I realized that I could rework my
library until it was independent of widget set, and allow Dave to write a UI to
sit on top of it, not bound by the fontforge license. So I reworked the
internals of fontforge to make them extensible, stripped the UI out of
libfontforge. And started to work on a gtk based fontforge of my own.

Dave Crossland was complaining on the Open Font Library mailing list about how
much information was lost when a font was released. Guidelines. Names of
lookups. Cubic splines used for generating the quadratics of TrueType. And about
the need for providing sources. Well, providing sources of fonts can be
difficult, and not always useful if the tools to generate the fonts aren't also
available. However there is no reason why much of that information can't be
stored in the font itself. I already had a table that FontForge would create
called ('PfEd', left over from PfaEdit days) which stored per-glyph comments,
and other things. I could simply extend that table to store guidelines and other
things. And document it so that others could use it, of course -- but I'd
already done that.

And that brings us up to Jan 2008, I guess the pace of change sped up a bit this
year as opposed to last.

Alexey and others complained that they wanted multiple layers of splines. More
than just the Foreground, Background, Guidelines layers that FontForge came
with. One common request was to have both a cubic (PostScript) and a quadratic
(TrueType) layer and be able to generate fonts from both. So in March of 2008
FontForge grew multiple layer support.

Later in March I added support for the OpenType 'BASE' and Apple 'bsln' tables.
And to amuse myself I added the ablity to have gradient fills in Type3 (and svg)
fonts.

In June I was thinking of the embolden command I did the year before, and
realized that that was essentially the same idea as was needed for generating
Small Caps glyphs from Capital letters. And then some of those algorithms could
be used to create condensed and extended glyphs. And then I sat down and wrote a
generic "glyph change" dialog -- years ago I had had a "MetaFont" command which
was supposed to allow the user to embolden fonts for condense them or ...
Unfortunately my MetaFont never worked very well (And some users complained that
It didn't read Knuth's mf files. Sigh. No, it metamorphosed fonts in its own
way, not Knuth's), so it got removed. Now it was basically working in a new form
-- but I know better than to call it MetaFont now.

Alexey then stepped in and rewrote much of the code. I did not handle diagonal
stems well when creating small caps, and that was just what he was doing with
the autohinter. So he greatly improved the output.

I was also intregued by italics. Converting a font to italics involves so many
different things -- the font is slanted, compressed, serifs change, letter forms
change, ... I studied existing fonts to see what I could learn and asked
various real typographers. The consensus I heard from them was that I could
never make a good italic font from a roman one mechanically and should not
bother to try -- it would just lead to people making bad italic fonts. Good
advice, but I didn't follow it. I thought it was a neat challenge. And it was
something Ikarus had done, so I wanted to do it too.

In July a friend of mine, who is a mac user, said she wouldn't even consider
looking at fontforge on her mac unless it behaved more like a mac application.
So I figured out how to build a mac Application, and how to respond to apple
events (like having someone double click on a font file, or drop a font file on
fontforge's icon). I figured out how to start up X so that the user didn't have
to. I made pretty (well, I think they are pretty) icons for font files. I even
changed the menus to use the command key on the mac and to show the mac
cloverleaf icon.

My friend still (November) has not looked at fontforge. Ah well.

Dave Crossland had hired someone to integrate cairo into fontforge. But the
result never got back to me. In a moment of foolish boredom I decided I could do
that too. So I studied cairo, and it really didn't seem that hard. But it was
slow -- at least on my 10 year old x86 machine which doesn't support XRender.
Cairo gave two things I cared about, anti-aliased splines in the glyph view, and
anti-aliased text everywhere. Well I needed cairo in the glyph view, but pango
would also provide fuzzy text and was lighter weight and would also support
complex scripts (which fontforge's own widget set did not do). So I could turn
off cairo everywhere but the glyph view but still get fuzzy text from pango. And
speed things up. Then Khaled Hosny suggested that I implement pango. Hurumph.
And I had wanted to surprise people. Oh well. Implement Pango I did.

A group in Japan created the "Unofficial mingw fontforge page". A very nice
piece of work. It included a set of X resources which provided another, nice
look to the UI. A theme. And then other people started writing themes -- and
started complaining about and finding old bugs in fontforge's resource reading
code -- it had never been exercised before I guess.

--------------------------------------------------------------------------------

I have received many suggestions from many people, too many to enumerate here,
and FontForge is the better for their requests. Often I have reacted badly to
these suggestions (because they always mean more work for me), and I apologize
for that, but mostly I wish to thank those who have helped make FontForge what
it is today.

Currently, probably the biggest complaint about FontForge is the choice of
widget set. No one likes my widgets (except me). Unfortunately for the rest of
the world
:ref:`I don't like the two choices of widget set available to me (gtk and qt) <faq.widget-set>`.
I will get started working on converting to one and then run into some problem I
can't work around easily and give up and go back to my own. Well in 2008 I still
don't like gtk, but I have the fontview working in it. A start but probably not
something I will continue.