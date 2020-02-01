Error Recovery
==============

All programs crash, and new programs (like this one) crash a lot.

Every minute or so FontForge looks around and sees if anything has changed since
the last time it checked. If it finds a font that has had changes, it will save
a small file in its private directory (``~/.FontForge/autosave``) which
consists of all changed glyphs in the changed font.

When FontForge starts up it checks this directory, and if it finds anything
there it attempts to apply those changes to the original font. So if FontForge
crashes, or if your machine crashes, the next time you start FontForge it will
figure out what you were doing and recover it as best it can.

When editing CID-keyed fonts there are a few potential gotchas:

* FontForge does not keep track of gross changes (like adding or removing a sub
  font)
* FontForge does not keep track of which sub font a change occurred in, instead
  at recovery time it will look for a subfont that is contains a glyph at the
  CID that was changed. In almost all cases this will be right, but if you add
  glyphs or if you have two sub-fonts defining the same CID, FontForge may guess
  wrong.