#Needs: fonts/Zapfino-4.1d6.dfont

import os, sys, shutil, tempfile, fontforge

results = tempfile.mkdtemp('.tmp','fontforge-test-')

print("This font has odd 'morx' tables, we just don't want to crash")
zapfino = fontforge.open(sys.argv[1])
zapfino.generate(os.path.join(results, "Zapfino.ttf"),flags=("apple",))
zapfino.close()

shutil.rmtree(results)
