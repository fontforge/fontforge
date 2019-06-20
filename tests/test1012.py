import sys, fontforge

font = fontforge.open(sys.argv[1])
a = font["uni0627"]
b = font["uni0628"]

assert a.getPosSub("*")[0][2:] == ("uni0628", -50, 0, -50, 0, 0, 0, 0, 0), a.getPosSub("*")[0][2:]
assert a.getPosSub("*")[1][2:] == ("uni0629",   0, 0, -50, 0, 0, 0, 0, 0), a.getPosSub("*")[1][2:]
assert a.getPosSub("*")[2][2:] == ("uni0628", -50, 0, -50, 0, 0, 0, 0, 0), a.getPosSub("*")[0][2:]
assert a.getPosSub("*")[3][2:] == ("uni0629",   0, 0, -50, 0, 0, 0, 0, 0), a.getPosSub("*")[1][2:]
assert b.getPosSub("*")[0][2:] == ("uni0629", -50, 0,   0, 0, 0, 0, 0, 0), b.getPosSub("*")[0][2:]
assert b.getPosSub("*")[1][2:] == ("uni0629", -50, 0,   0, 0, 0, 0, 0, 0), b.getPosSub("*")[0][2:]

font.generate("test1012.result.otf")

font2 = fontforge.open("test1012.result.otf")
a = font2["uni0627"]
b = font2["uni0628"]

assert a.getPosSub("*")[0][2:] == ("uni0628", -50, 0, -50, 0, 0, 0, 0, 0), a.getPosSub("*")[0][2:]
assert a.getPosSub("*")[1][2:] == ("uni0629",   0, 0, -50, 0, 0, 0, 0, 0), a.getPosSub("*")[1][2:]
assert a.getPosSub("*")[2][2:] == ("uni0628", -50, 0, -50, 0, 0, 0, 0, 0), a.getPosSub("*")[0][2:]
assert a.getPosSub("*")[3][2:] == ("uni0629",   0, 0, -50, 0, 0, 0, 0, 0), a.getPosSub("*")[1][2:]
assert b.getPosSub("*")[0][2:] == ("uni0629", -50, 0,   0, 0, 0, 0, 0, 0), b.getPosSub("*")[0][2:]
assert b.getPosSub("*")[1][2:] == ("uni0629", -50, 0,   0, 0, 0, 0, 0, 0), b.getPosSub("*")[0][2:]
