#!/usr/local/bin/fontforge
#Needs: fonts/nuvo-medium-woff-demo.woff

import fontforge;
woff=fontforge.open("fonts/nuvo-medium-woff-demo.woff");
if ( woff.woffMajor!=7 ) :
  raise ValueError, "Wrong return from woffMajor"
if ( woff.woffMinor!=504 ) :
  raise ValueError, "Wrong return from woffMinor"
if ( len(woff.woffMetadata)!=959 ) :
  raise ValueError, "Wrong return from woffMetadata"

woff.woffMajor = 8;

woff.generate("results/Foo.woff");

wofftest=fontforge.open("results/Foo.woff");
if ( wofftest.woffMajor!=8 | wofftest.woffMinor!=504 ) :
  raise ValueError, "Wrong return from woffMajor woffMinor after saving"
if ( len(wofftest.woffMetadata)!=959 ) :
  raise ValueError, "Wrong return from woffMetadata after saving"
