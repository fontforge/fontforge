#Test the math table

import fontforge

font=fontforge.font()
math = font.math

if math.exists():
  raise ValueError("Thinks there is a math table in an empty font")
math.clear()

math.ScriptPercentScaleDown = 3
math.SubscriptBaselineDropMin = 6
if not math.exists():
  raise ValueError("Thinks there isn't a math table after we added one")
if ( math.ScriptPercentScaleDown!=3 or math.SubscriptBaselineDropMin != 6) :
  raise ValueError("Assignment failed")

# Test some default values for math constants
math.clear()
if ( math.ScriptPercentScaleDown != 80 or
     math.ScriptScriptPercentScaleDown != 60 or
     math.DelimitedSubFormulaMinHeight != int(round(font.em*1.5)) or
     math.SubSuperscriptGapMin != int(round(4*font.uwidth)) or
     math.SpaceAfterScript != int(round(font.em/24)) or
     math.StackGapMin != int(round(3*font.uwidth)) or
     math.StackDisplayStyleGapMin != int(round(7*font.uwidth)) or
     math.FractionNumeratorGapMin != int(round(font.uwidth)) or
     math.FractionNumeratorDisplayStyleGapMin != int(round(3*font.uwidth)) or
     math.FractionRuleThickness != int(round(font.uwidth)) or
     math.FractionDenominatorGapMin != int(round(font.uwidth)) or
     math.OverbarVerticalGap != int(round(3*font.uwidth)) or
     math.OverbarRuleThickness != int(round(font.uwidth)) or
     math.OverbarExtraAscender != int(round(font.uwidth)) or
     math.UnderbarVerticalGap != int(round(3*font.uwidth)) or
     math.UnderbarRuleThickness != int(round(font.uwidth)) or
     math.UnderbarExtraDescender != int(round(font.uwidth)) or
     math.RadicalVerticalGap != int(round(font.uwidth)) or
     math.RadicalDisplayStyleVerticalGap != int(round(font.uwidth)) or
     math.RadicalRuleThickness != int(round(font.uwidth)) or
     math.RadicalRuleThickness != int(round(font.uwidth)) or
     math.RadicalExtraAscender != int(round(font.uwidth)) or
     math.RadicalKernBeforeDegree != int(round(5*font.em/18)) or
     math.RadicalKernAfterDegree != -int(round(10*font.em/18)) or
     math.RadicalDegreeBottomRaisePercent != 60 ):
  raise ValueError("Unexpected default value for one math constant")

math.clear()
if math.exists():
  raise ValueError("Thinks there is a math table in an empty font")

a = font.createChar(65)
c = font.createChar(67)
a.horizontalVariants = "B C D"
a.horizontalComponents = (("a",),("b",1,20,20,200),c)
a.horizontalComponentItalicCorrection = 10

a.verticalVariants = "B.v C.v D.v"
a.verticalComponents = (("a",),("b",1,20,20,200),c)
a.verticalComponentItalicCorrection = 20

if a.horizontalVariants != "B C D" or a.horizontalComponentItalicCorrection!=10:
  raise ValueError("Failed to set some glyph horizontal variant/component")
if a.verticalVariants != "B.v C.v D.v" or a.verticalComponentItalicCorrection!=20:
  raise ValueError("Failed to set some glyph vertical variant/component")

# non-regression test for a bad allocation in ParseComponentTuple
font[65].horizontalComponents = (('a',))
font[65].verticalComponents = (('a',))

#print a.verticalComponents
#print a.mathKern.topLeft

#a.mathKern.topLeft = ((1,2),(2,3))
#print a.mathKern.topLeft
#print a.mathKern.topRight
