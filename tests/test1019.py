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
    assert len(font.math.MathLeadingDeviceTable) == len(non_zero_adjustments)

# Set table to None
font.math.AxisHeightDeviceTable = {10: -1}
# Check that None assignment removes all data from table
font.math.AxisHeightDeviceTable = None
assert font.math.AxisHeightDeviceTable == {}

# Set element to None
font.math.AxisHeightDeviceTable = {10: 3, 11:-1}
assert font.math.AxisHeightDeviceTable[11] == -1
font.math.AxisHeightDeviceTable[11] = None
assert font.math.AxisHeightDeviceTable[11] == 0

# Directly assign value to a table entry
font.math.MathLeadingDeviceTable[9] = -1
font.math.MathLeadingDeviceTable[10] = 0
font.math.MathLeadingDeviceTable[12] = 2

# Verify all non-zero values were assigned
assert font.math.MathLeadingDeviceTable == {9: -1, 12: 2}
assert len(font.math.MathLeadingDeviceTable) == 2

# Verify access by pixel size
assert font.math.MathLeadingDeviceTable[3] == 0
assert font.math.MathLeadingDeviceTable[9] == -1
assert font.math.MathLeadingDeviceTable[11] == 0
assert font.math.MathLeadingDeviceTable[12] == 2
assert font.math.MathLeadingDeviceTable[20] == 0

# Copy device table to another entry
assert font.math.SpaceAfterScriptDeviceTable == {}
font.math.SpaceAfterScriptDeviceTable = font.math.MathLeadingDeviceTable
assert font.math.SpaceAfterScriptDeviceTable == {9: -1, 12: 2}

# Check comparison
assert not font.math.SpaceAfterScriptDeviceTable == {12: 2}
assert font.math.SpaceAfterScriptDeviceTable != {12: 2}
