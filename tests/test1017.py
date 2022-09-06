#Needs: fonts/invalid-gsub.fea

#Test to ignore invalid substitution lookups when merging feature file
import sys, fontforge

font = fontforge.font()
feat = sys.argv[1]

A = font.createChar(ord("A"), "A")
B = font.createChar(ord("B"), "B")
a = font.createChar(ord("a"), "a")
b = font.createChar(ord("b"), "b")
f = font.createChar(ord("f"), "f")
l = font.createChar(ord("l"), "l")

#font.mergeFeature(feat)
font.mergeFeature(feat, True)

#No error
sys.exit(0)
