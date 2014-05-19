#Needs: fonts/nuvo-medium-woff-demo.woff

import os, sys, shutil, tempfile, fontforge

results = tempfile.mkdtemp('.tmp','fontforge-test-')

woff=fontforge.open(sys.argv[1])
if woff.woffMajor!=7:
  raise ValueError("Wrong return from woffMajor")
if woff.woffMinor!=504:
  raise ValueError("Wrong return from woffMinor")
metadata_len = len(woff.woffMetadata)
if metadata_len!=959 and metadata_len!=957: # Different lengths found in different versions
  raise ValueError("Wrong return from woffMetadata")

woff.woffMajor = 8

woff.generate(os.path.join(results, "Foo.woff"))

wofftest=fontforge.open(os.path.join(results, "Foo.woff"))
if wofftest.woffMajor!=8 or wofftest.woffMinor!=504:
  raise ValueError("Wrong return from woffMajor woffMinor after saving")
if len(wofftest.woffMetadata)!=metadata_len:
  raise ValueError("Wrong return from woffMetadata after saving")

shutil.rmtree(results)
