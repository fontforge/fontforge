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

# Create a glyph for the 'x' letter, so that math->AccentBaseHeight is nonzero.
accentBaseHeight = font.em * .75
g = font.createChar(0x78)
p = g.glyphPen();
p.moveTo(0, 0);
p.lineTo(accentBaseHeight, 0);
p.lineTo(accentBaseHeight, accentBaseHeight);
p.lineTo(0, accentBaseHeight)
p.closePath();

# Verify whether some glyph constants are unspecified by default.
if ( g.texheight != fontforge.unspecifiedMathValue or
     g.texdepth != fontforge.unspecifiedMathValue or
     g.topaccent != fontforge.unspecifiedMathValue or
     g.italicCorrection != fontforge.unspecifiedMathValue ):
  raise ValueError("Unexpected default value for a glyph constant")

# Create a glyph for the '+' letter, so that math->AxisHeight is nonzero.
axisHeight = font.em * .3
g = font.createChar(0x2B)
p = g.glyphPen();
p.moveTo(0, 0);
p.lineTo(2*axisHeight, 0);
p.lineTo(2*axisHeight, 2*axisHeight);
p.lineTo(0, 2*axisHeight)
p.closePath();

if ( math.ScriptPercentScaleDown != 80 or
     math.ScriptScriptPercentScaleDown != 60 or
     math.DelimitedSubFormulaMinHeight != int(font.em*1.5) or
     math.AxisHeight != int(axisHeight) or
     math.SubscriptTopMax != int(accentBaseHeight) or
     math.SuperscriptBottomMin != int(accentBaseHeight) or
     math.SubSuperscriptGapMin != int(4*font.uwidth) or
     math.SuperscriptBottomMaxWithSubscript != int(accentBaseHeight) or
     math.SpaceAfterScript != int(font.em/24) or
     math.StackGapMin != int(3*font.uwidth) or
     math.StackDisplayStyleGapMin != int(7*font.uwidth) or
     math.FractionNumeratorGapMin != int(font.uwidth) or
     math.FractionNumeratorDisplayStyleGapMin != int(3*font.uwidth) or
     math.FractionRuleThickness != int(font.uwidth) or
     math.FractionDenominatorGapMin != int(font.uwidth) or
     math.OverbarVerticalGap != int(3*font.uwidth) or
     math.OverbarRuleThickness != int(font.uwidth) or
     math.OverbarExtraAscender != int(font.uwidth) or
     math.UnderbarVerticalGap != int(3*font.uwidth) or
     math.UnderbarRuleThickness != int(font.uwidth) or
     math.UnderbarExtraDescender != int(font.uwidth) or
     math.RadicalVerticalGap != int(font.uwidth) or
     math.RadicalDisplayStyleVerticalGap != int(font.uwidth+.25*accentBaseHeight) or
     math.RadicalRuleThickness != int(font.uwidth) or
     math.RadicalRuleThickness != int(font.uwidth) or
     math.RadicalExtraAscender != int(font.uwidth) or
     math.RadicalKernBeforeDegree != int(5*font.em/18) or
     math.RadicalKernAfterDegree != -int(10*font.em/18) or
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
