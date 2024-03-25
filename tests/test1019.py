#Needs: fonts/Ambrosia.sfd

#Test MATH device table access
import sys, fontforge

ambrosia = sys.argv[1]
font = fontforge.open(ambrosia)

assert font.math.MathLeadingDeviceTable == {}

# Assign various adjustment sets
for adjustments in [
    {9: -1, 10: -1, 12: -1}, # set non-empty
    {9: -1, 10: -1, 12: -1}, # set previous
    {9: -1, 10: -1}, # reduce
    {9: -1, 10: -1, 14: -1, 18: -1}, # increase
    {9: 1, 10: -2, 12: 0, 18: -3}, # change values
    {9: 1, 12: 0, 18: -3, 10: -2}, # change order
    {}, # set empty
    {} # set previous
]:
    font.math.MathLeadingDeviceTable = adjustments
    # Filter out zero entries, they are not stored
    non_zero_adjustments = {k:v for (k,v) in adjustments.items() if v != 0}
    assert font.math.MathLeadingDeviceTable == non_zero_adjustments
