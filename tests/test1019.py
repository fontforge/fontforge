#Needs: fonts/Ambrosia.sfd

#Test MATH device table access
import sys, fontforge

ambrosia = sys.argv[1]
font = fontforge.open(ambrosia)

assert font.math.MathLeadingDeviceTable == ()

for adjustments in [
    ((9, -1), (10, -1), (12, -1)), # set non-empty
    ((9, -1), (10, -1), (12, -1)), # set previous
    ((9, -1), (10, -1)), # reduce
    ((9, -1), (10, -1), (14, -1), (18, -1)), # increase
    ((9, 1), (10, -2), (12, 0), (18, -3)), # change values
    ((9, 1), (12, 0), (18, -3), (10, -2)), # change order
    (), # set empty
    () # set previous
]:
    font.math.MathLeadingDeviceTable = adjustments
    adjustments = sorted(adjustments, key=lambda adj: adj[0])
    adjustments = tuple((p, c) for p, c in adjustments if c != 0) # cor == 0 entries are not returned
    assert font.math.MathLeadingDeviceTable == adjustments
