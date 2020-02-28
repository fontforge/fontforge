X Input Methods
===============

For entering text in CJK languages, simple keyboard typing is not adequate.
Instead there are programs that run and handle the conversion of keystrokes to
characters these programs are called input methods.

I have tested FontForge with two freely available input methods (as well as one
can who speaks neither Chinese, Japanese nor Korean) kinput2 (for Japanese) and
`xcin <http://xcin.linux.org.tw/>`__ (for Chinese).

There is reasonably good (English) documentation on installing and using kinput2
on the mozilla site, and at
`suse <http://www.suse.de/~mfabian/suse-cjk/kinput2.html>`__, kinput2 has the
interesting complexity that it requires yet another server to be running,
generally either cannaserver or jserver. It looks to me as though it might be
possible to use a chinese or korean jserver with kinput2 but I have not tried
this.

There is good Chinese and English documentation on xcin at the
`xcin site in Taiwan <http://xcin.linux.org.tw/>`__ (english is not the default
here, but it is available about 3 lines down).

One of the most difficult problems I had in installing these was finding the
appropriate locales. I could not find them in my RedHat 7.3 distribution, nor
could I find any RedHad rpms containing them. There is a good supply of Mandrake
locale rpms (named ``locales-zh*`` for chinese, ``locales-jp*`` for japanese,
etc.) but Mandrake stores them in a different directory so after installing them
I had to copy them from ``/usr/share/locales`` to ``/usr/lib/locales``. The SUSE
docs imply that the current SUSE distribution ships with these locales.

To start fontforge with an input method you must first insure that the input
method itself is running. For kinput2 do the following:

::

   $ cannaserver &
   $ kinput2 -canna -xim &

While for xcin:

::

   $ setenv LC_CTYPE zh_TW.Big5             (or $ LC_CTYPE=zh_TW.Big5 ; export LC_CTYPE )
   $ xcin &

Beware: Different systems will have slightly different locale names (sometimes
it's ``zh_TW.big5``, or something else) so if things don't work try something
else.

Once you've started your input method you must then start FontForge. You should
set the LC_CTYPE environment variable (or LC_ALL or LANG) to the appropriate
locale. You may (if you have multiple input methods running that support the
same locale) need to set the XMODIFIERS environment variable to the name of your
method (xcin can have multiple names, it prints out the one it is currently
using on start-up).

::

   $ setenv LC_CTYPE ja_JP.eucJP
   $ setenv XMODIFIERS "@im=kinput2"
   $ fontforge -new

FontForge will start up significantly more slowly when connecting to an IM, just
be patient with it. FontForge supports OverTheSpot input in textfields and Root
input in the font and outline character views. Not all textfields accept unicode
input, but any that do should now accept input from an IM.

.. note:: 

   This only works if ``X_HAVE_UTF8_STRING`` is defined -- an XFree 4.0.2
   extension (ie. not on solaris)

--------------------------------------------------------------------------------

If you are interested in the mechanics of XIM programming I refer you to
O'Reilly's excellent
`Programmers Reference Guide for X11 <http://capderec.udg.es:81/ebt-bin/nph-dweb/dynaweb/SGI_Developer/XLib_PG/@Generic__BookView>`__,
chapter 11.