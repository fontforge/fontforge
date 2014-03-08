import os, shutil, tempfile, fontforge

results = tempfile.mkdtemp('.tmp','fontforge-test-')

newfont = fontforge.font()
newfont.save(os.path.join(results, "foo.sfd"))
print("...Saved new font")
newfont.close()

shutil.rmtree(results)
