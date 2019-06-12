import sys, fontforge

font = fontforge.font()
feat = sys.argv[1]

A = font.createChar(ord("A"), "A")
pen = A.glyphPen()
pen.moveTo((100,100))
pen.lineTo((100,200))
pen.lineTo((200,200))
pen.lineTo((200,100))
pen.closePath()

B = font.createChar(ord("B"), "B")
pen = B.glyphPen()
pen.moveTo((100,100))
pen.lineTo((100,200))
pen.lineTo((200,200))
pen.lineTo((200,100))
pen.closePath()

font.mergeFeature(feat)

expected = (0, 0, 100, 0, 0, 0, 0, 0)
assert A.getPosSub(font.getLookupSubtables("ltr")[0])[0][3:] == expected
assert A.getPosSub(font.getLookupSubtables("rtl")[0])[0][3:] == expected

font.generateFeatureFile("test1011.result.fea")

with open(feat) as expected_file:
    with open("test1011.result.fea") as result_file:
        expected = expected_file.readlines()
        result = result_file.readlines()
        if result != expected:
            from difflib import Differ
            d = Differ()
            sys.stderr.writelines(list(d.compare(result, expected)))
            sys.exit(1)
sys.exit(0)
