#Needs: fonts/Zapfino-4.0d3.dfont

import os, sys, shutil, tempfile, fontforge

results = tempfile.mkdtemp('.tmp','fontforge-test-')

zapfino = fontforge.open(sys.argv[1])
zapfino.generate(os.path.join(results, "Zapfino.ttf"),flags=("apple",))
zapfino.close()
#We used to get PointCount errors in the above.
zapfino = fontforge.open(os.path.join(results, "Zapfino.ttf"))
zapfino.close()
print("...Handled Zapfino with control points at points")

shutil.rmtree(results)
