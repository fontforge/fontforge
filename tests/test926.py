#Needs: fonts/DejaVuSerif.sfd
#
################################################################################
########################### W3C's WOFF Validator ###############################
################################################################################
#
# This software or document includes material copied from or derived from
# http://dev.w3.org/webfonts/WOFF/tools/validator/
# Copyright (C) 2012 W3C(R) (MIT, ERCIM, Keio, Beihang).
#
# This work is being provided by the copyright holders under the following
# license.
#
# License
#
# By obtaining and/or copying this work, you (the licensee) agree that you have
# read, understood, and will comply with the following terms and conditions.
#
# Permission to copy, modify, and distribute this work, with or without
# modification, for any purpose and without fee or royalty is hereby granted,
# provided that you include the following on ALL copies of the work or portions
# thereof, including modifications:
#
#     The full text of this NOTICE in a location viewable to users of the
# redistributed or derivative work.
#     Any pre-existing intellectual property disclaimers, notices, or terms and
# conditions. If none exist, the W3C Software and Document Short Notice should
# be included.
#     Notice of any changes or modifications, through a copyright statement on
# the new code or document such as "This software or document includes material
# copied from or derived from [title and URI of the W3C document].
# Copyright (C) [YEAR] W3C(R) (MIT, ERCIM, Keio, Beihang)."
#
# Disclaimers
#
# THIS WORK IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS
# OR WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO, WARRANTIES
# OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF
# THE SOFTWARE OR DOCUMENT WILL NOT INFRINGE ANY THIRD PARTY PATENTS,
# COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
#
# COPYRIGHT HOLDERS WILL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL OR
# CONSEQUENTIAL DAMAGES ARISING OUT OF ANY USE OF THE SOFTWARE OR DOCUMENT.
#
# The name and trademarks of copyright holders may NOT be used in advertising
# or publicity pertaining to the work without specific, written prior
# permission. Title to copyright in this work will at all times remain with
# copyright holders.
#

"""
A module for validating the the file structure of WOFF Files.
*validateFont* is the only public function.

This can also be used as a command line tool for validating WOFF files.
"""

# import

import fontforge
import os
import re
import time
import sys
import struct
import zlib
import optparse
import codecs
from xml.etree import ElementTree
from xml.parsers.expat import ExpatError
try:
    from cStringIO import StringIO
except ImportError:
    from io import StringIO

# ----------------------
# Support: Metadata Spec
# ----------------------

"""
The Extended Metadata specifications are defined as a set of
nested Python objects. This allows for a very simple XML
validation procedure. The common element structure is as follows:

    {
        # ----------
        # Attributes
        # ----------

        # In all cases, the dictionary has the attribute name at the top
        # with the possible value(s) as the value. If an attribute has
        # more than one representation (for exmaple xml:lang and lang)
        # the two are specified as a space separated string for example
        # "xml:lang lang".

        # Required
        "requiredAttributes" : {
            # empty or one or more of the following
            "name" : "default as string, list of options or None"
        },

        # Recommended
        "recommendedAttributes" : {
            # empty or one or more of the following
            "name" : "default as string, list of options or None"
        },

        # Optional
        "optionalAttributes" : {
            # empty or one or more of the following
            "name" : "default as string, list of options or None"
        },

        # -------
        # Content
        # -------

        "contentLevel" : "not allowed", "recommended" or "required",

        # --------------
        # Child Elements
        # --------------

        # In all cases, the dictionary has the element name at the top
        # with a dictionary as the value. The value dictionary defines
        # the number of times the shild-element may occur along with
        # the specification for the child-element.

        # Required
        "requiredChildElements" : {
            # empty or one or more of the following
            "name" : {
                "minimumOccurrences" : int or None,
                "maximumOccurrences" : int or None,
                "spec" : {}
            }
        },

        # Recommended
        "recommendedChildElements" : {
            # empty or one or more of the following
            "name" : {
                # minimumOccurrences is implicitly 0
                "maximumOccurrences" : int or None,
                "spec" : {}
            }
        },

        # Optional
        "optionalChildElements" : {
            # empty or one or more of the following
            "name" : {
                # minimumOccurrences is implicitly 0
                "maximumOccurrences" : int or None,
                "spec" : {}
            }
        }
    }

The recommendedAttributes and recommendedChildElements are optional
but they are separated from the optionalAttributes and optionalChildElements
to allow for more detailed reporting.
"""

# Metadata 1.0
# ------------

# Common Options

dirOptions_1_0 = ["ltr", "rtl"]

# Fourth-Level Elements

divSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "dir" : dirOptions_1_0,
        "class" : None
    },
    "content" : "recommended",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {
        "div" : {
            "maximumOccurrences" : None,
            "spec" : "recursive divSpec_1_0" # special override for recursion.
        },
        "span" : {
            "maximumOccurrences" : None,
            "spec" : "recursive spanSpec_1_0" # special override for recursion.
        }
    }
}

spanSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "dir" : dirOptions_1_0,
        "class" : None
    },
    "content" : "recommended",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {
        "div" : {
            "maximumOccurrences" : None,
            "spec" : "recursive divSpec_1_0" # special override for recursion.
        },
        "span" : {
            "maximumOccurrences" : None,
            "spec" : "recursive spanSpec_1_0" # special override for recursion.
        }
    }
}

# Third-Level Elements

creditSpec_1_0 = {
    "requiredAttributes" : {
        "name" : None
    },
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "url" : None,
        "role" : None,
        "dir" : dirOptions_1_0,
        "class" : None
    },
    "content" : "not allowed",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

textSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "url" : None,
        "role" : None,
        "dir" : dirOptions_1_0,
        "class" : None,
        "xml:lang lang" : None
    },
    "content" : "recommended",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {
        "div" : {
            "maximumOccurrences" : None,
            "spec" : divSpec_1_0
        },
        "span" : {
            "maximumOccurrences" : None,
            "spec" : spanSpec_1_0
        }
    }
}

extensionNameSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "dir" : dirOptions_1_0,
        "class" : None,
        "xml:lang lang" : None
    },
    "content" : "recommended",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

extensionValueSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "dir" : dirOptions_1_0,
        "class" : None,
        "xml:lang lang" : None
    },
    "content" : "recommended",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

extensionItemSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "id" : None
    },
    "content" : "not allowed",
    "requiredChildElements" : {
        "name" : {
            "minimumOccurrences" : 1,
            "maximumOccurrences" : None,
            "spec" : extensionNameSpec_1_0
        },
        "value" : {
            "minimumOccurrences" : 1,
            "maximumOccurrences" : None,
            "spec" : extensionValueSpec_1_0
        }
    },
    "recommendedChildElements" : {
    },
    "optionalChildElements" : {}
}

# Second Level Elements

uniqueidSpec_1_0 = {
    "requiredAttributes" : {
        "id" : None
    },
    "recommendedAttributes" : {},
    "optionalAttributes" : {},
    "content" : "not allowed",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

vendorSpec_1_0 = {
    "requiredAttributes" : {
        "name" : None
    },
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "url" : None,
        "dir" : dirOptions_1_0,
        "class" : None
    },
    "content" : "not allowed",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

creditsSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {},
    "content" : "not allowed",
    "requiredChildElements" : {
        "credit" : {
            "minimumOccurrences" : 1,
            "maximumOccurrences" : None,
            "spec" : creditSpec_1_0
        }
    },
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

descriptionSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "url" : None,
    },
    "content" : "not allowed",
    "requiredChildElements" : {
        "text" : {
            "minimumOccurrences" : 1,
            "maximumOccurrences" : None,
            "spec" : textSpec_1_0
        }
    },
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

licenseSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "url" : None,
        "id" : None
    },
    "content" : "not allowed",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {
        "text" : {
            "maximumOccurrences" : None,
            "spec" : textSpec_1_0
        }
    }
}

copyrightSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {},
    "content" : "not allowed",
    "requiredChildElements" : {
        "text" : {
            "minimumOccurrences" : 1,
            "maximumOccurrences" : None,
            "spec" : textSpec_1_0
        }
    },
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

trademarkSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {},
    "content" : "not allowed",
    "requiredChildElements" : {
        "text" : {
            "minimumOccurrences" : 1,
            "maximumOccurrences" : None,
            "spec" : textSpec_1_0
        }
    },
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

licenseeSpec_1_0 = {
    "requiredAttributes" : {
        "name" : None,
    },
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "dir" : dirOptions_1_0,
        "class" : None
    },
    "content" : "not allowed",
    "requiredChildElements" : {},
    "recommendedChildElements" : {},
    "optionalChildElements" : {}
}

extensionSpec_1_0 = {
    "requiredAttributes" : {},
    "recommendedAttributes" : {},
    "optionalAttributes" : {
        "id" : None
    },
    "content" : "not allowed",
    "requiredChildElements" : {
        "item" : {
            "minimumOccurrences" : 1,
            "maximumOccurrences" : None,
            "spec" : extensionItemSpec_1_0
        }
    },
    "recommendedChildElements" : {},
    "optionalChildElements" : {
        "name" : {
            "maximumOccurrences" : None,
            "spec" : extensionNameSpec_1_0
        }
    }
}

# First Level Elements

metadataSpec_1_0 = {
    "requiredAttributes" : {
        "version" : "1.0"
    },
    "recommendedAttributes" : {},
    "optionalAttributes" : {},
    "content" : "not allowed",
    "requiredChildElements" : {},
    "recommendedChildElements" : {
        "uniqueid" : {
            "maximumOccurrences" : 1,
            "spec" : uniqueidSpec_1_0
        }
    },
    "optionalChildElements" : {
        "vendor" : {
            "maximumOccurrences" : 1,
            "spec" : vendorSpec_1_0
        },
        "credits" : {
            "maximumOccurrences" : 1,
            "spec" : creditsSpec_1_0
        },
        "description" : {
            "maximumOccurrences" : 1,
            "spec" : descriptionSpec_1_0
        },
        "license" : {
            "maximumOccurrences" : 1,
            "spec" : licenseSpec_1_0
        },
        "copyright" : {
            "maximumOccurrences" : 1,
            "spec" : copyrightSpec_1_0
        },
        "trademark" : {
            "maximumOccurrences" : 1,
            "spec" : trademarkSpec_1_0
        },
        "licensee" : {
            "maximumOccurrences" : 1,
            "spec" : licenseeSpec_1_0
        },
        "licensee" : {
            "maximumOccurrences" : 1,
            "spec" : licenseeSpec_1_0
        },
        "extension" : {
            "maximumOccurrences" : None,
            "spec" : extensionSpec_1_0
        }
    }
}

# ----------------------
# Support: struct Helper
# ----------------------

# This was inspired by Just van Rossum's sstruct module.
# http://fonttools.svn.sourceforge.net/svnroot/fonttools/trunk/Lib/sstruct.py

def structPack(format, obj):
    keys, formatString = _structGetFormat(format)
    values = []
    for key in keys:
        values.append(obj[key])
    data = struct.pack(formatString, *values)
    return data

def structUnpack(format, data):
    keys, formatString = _structGetFormat(format)
    size = struct.calcsize(formatString)
    values = struct.unpack(formatString, data[:size])
    unpacked = {}
    for index, key in enumerate(keys):
        value = values[index]
        unpacked[key] = value
    return unpacked, data[size:]

def structCalcSize(format):
    keys, formatString = _structGetFormat(format)
    return struct.calcsize(formatString)

_structFormatCache = {}

def _structGetFormat(format):
    if format not in _structFormatCache:
        keys = []
        formatString = [">"] # always big endian
        for line in format.strip().splitlines():
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            key, formatCharacter = line.split(":")
            key = key.strip()
            formatCharacter = formatCharacter.strip()
            keys.append(key)
            formatString.append(formatCharacter)
        _structFormatCache[format] = (keys, "".join(formatString))
    return _structFormatCache[format]

# -------------
# Tests: Header
# -------------

def testHeader(data, reporter):
    """
    Test the WOFF header.
    """
    functions = [
        _testHeaderSignature,
        _testHeaderFlavor,
        _testHeaderLength,
        _testHeaderReserved,
        _testHeaderTotalSFNTSize,
        _testHeaderNumTables
    ]
    nonStoppingError = False
    for function in functions:
        stoppingError, nsError = function(data, reporter)
        if nsError:
            nonStoppingError = True
        if stoppingError:
            return True, nonStoppingError
    return False, nonStoppingError


headerFormat = """
    signature:      4s
    flavor:         4s
    length:         L
    numTables:      H
    reserved:       H
    totalSfntSize:  L
    majorVersion:   H
    minorVersion:   H
    metaOffset:     L
    metaLength:     L
    metaOrigLength: L
    privOffset:     L
    privLength:     L
"""
headerSize = structCalcSize(headerFormat)

def _testHeaderStructure(data, reporter):
    """
    Tests:
    - Header must be the proper structure.
    """
    try:
        structUnpack(headerFormat, data)
        reporter.logPass(message="The header structure is correct.")
    except:
        reporter.logError(message="The header is not properly structured.")
        return True, False
    return False, False

def _testHeaderSignature(data, reporter):
    """
    Tests:
    - The signature must be "wOFF".
    """
    header = unpackHeader(data)
    signature = header["signature"]
    if signature != b"wOFF":
        reporter.logError(message="Invalid signature: %s." % signature)
        return True, False
    else:
        reporter.logPass(message="The signature is correct.")
    return False, False

def _testHeaderFlavor(data, reporter):
    """
    Tests:
    - The flavor should be OTTO, 0x00010000 or true. Warn if another value is found.
    - If the flavor is OTTO, the CFF table must be present.
    - If the flavor is not OTTO, the CFF must not be present.
    - If the directory cannot be unpacked, the flavor can not be validated. Issue a warning.
    """
    header = unpackHeader(data)
    flavor = header["flavor"]
    if flavor not in (b"OTTO", b"\000\001\000\000", b"true"):
        reporter.logWarning(message="Unknown flavor: %s." % flavor)
    else:
        try:
            tags = [table["tag"] for table in unpackDirectory(data)]
            if b"CFF " in tags and flavor != b"OTTO":
                reporter.logError(message="A \"CFF\" table is defined in the font and the flavor is not set to \"OTTO\".")
                return False, True
            elif b"CFF " not in tags and flavor == b"OTTO":
                reporter.logError(message="The flavor is set to \"OTTO\" but no \"CFF\" table is defined.")
                return False, True
            else:
                reporter.logPass(message="The flavor is a correct value.")
        except:
            reporter.logWarning(message="Could not validate the flavor.")
    return False, False

def _testHeaderLength(data, reporter):
    """
    Tests:
    - The length of the data must match the defined length.
    - The length of the data must be long enough for header and directory for defined number of tables.
    - The length of the data must be long enough to contain the table lengths defined in the directory,
      the metaLength and the privLength.
    """
    header = unpackHeader(data)
    length = header["length"]
    numTables = header["numTables"]
    minLength = headerSize + (directorySize * numTables)
    if length != len(data):
        reporter.logError(message="Defined length (%d) does not match actual length of the data (%d)." % (length, len(data)))
        return False, True
    if length < minLength:
        reporter.logError(message="Invalid length defined (%d) for number of tables defined." % length)
        return False, True
    directory = unpackDirectory(data)
    for entry in directory:
        compLength = entry["compLength"]
        if compLength % 4:
            compLength += 4 - (compLength % 4)
        minLength += compLength
    metaLength = privLength = 0
    if header["metaOffset"]:
        metaLength = header["metaLength"]
    if header["privOffset"]:
        privLength = header["privLength"]
    if privLength and metaLength % 4:
        metaLength += 4 - (metaLength % 4)
    minLength += metaLength + privLength
    if length < minLength:
        reporter.logError(message="Defined length (%d) does not match the required length of the data (%d)." % (length, minLength))
        return False, True
    reporter.logPass(message="The length defined in the header is correct.")
    return False, False

def _testHeaderReserved(data, reporter):
    """
    Tests:
    - The reserved bit must be set to 0.
    """
    header = unpackHeader(data)
    reserved = header["reserved"]
    if reserved != 0:
        reporter.logError(message="Invalid value in reserved field (%d)." % reserved)
        return False, True
    else:
        reporter.logPass(message="The value in the reserved field is correct.")
    return False, False

def _testHeaderTotalSFNTSize(data, reporter):
    """
    Tests:
    - The size of the unpacked SFNT data must be a multiple of 4.
    - The origLength values in the directory, with proper padding, must sum
      to the totalSfntSize in the header.
    """
    header = unpackHeader(data)
    directory = unpackDirectory(data)
    totalSfntSize = header["totalSfntSize"]
    isValid = True
    if totalSfntSize % 4:
        reporter.logError(message="The total sfnt size (%d) is not a multiple of four." % totalSfntSize)
        isValid = False
    else:
        numTables = header["numTables"]
        requiredSize = sfntHeaderSize + (numTables * sfntDirectoryEntrySize)
        for table in directory:
            origLength = table["origLength"]
            if origLength % 4:
                origLength += 4 - (origLength % 4)
            requiredSize += origLength
        if totalSfntSize != requiredSize:
            reporter.logError(message="The total sfnt size (%d) does not match the required sfnt size (%d)." % (totalSfntSize, requiredSize))
            isValid = False
    if isValid:
        reporter.logPass(message="The total sfnt size is valid.")
    return False, not isValid

def _testHeaderNumTables(data, reporter):
    """
    Tests:
    - The number of tables must be at least 1.
    - The directory entries for the specified number of tables must be properly formatted.
    """
    header = unpackHeader(data)
    numTables = header["numTables"]
    if numTables < 1:
        reporter.logError(message="Invalid number of tables defined in header structure (%d)." % numTables)
        return False, True
    data = data[headerSize:]
    for index in range(numTables):
        try:
            d, data = structUnpack(directoryFormat, data)
        except:
            reporter.logError(message="The defined number of tables in the header (%d) does not match the actual number of tables (%d)." % (numTables, index))
            return False, True
    reporter.logPass(message="The number of tables defined in the header is valid.")
    return False, False

# -------------
# Tests: Tables
# -------------

def testDataBlocks(data, reporter):
    """
    Test the WOFF data blocks.
    """
    functions = [
        _testBlocksOffsetLengthZero,
        _testBlocksPositioning
    ]
    nonStoppingError = False
    for function in functions:
        stoppingError, nsError = function(data, reporter)
        if nsError:
            nonStoppingError = True
        if stoppingError:
            return True, nonStoppingError
    return False, nonStoppingError

def _testBlocksOffsetLengthZero(data, reporter):
    """
    - The metadata must have the offset and length set to zero consistently.
    - The private data must have the offset and length set to zero consistently.
    """
    header = unpackHeader(data)
    haveError = False
    # metadata
    metaOffset = header["metaOffset"]
    metaLength = header["metaLength"]
    if metaOffset == 0 or metaLength == 0:
        if metaOffset == 0 and metaLength == 0:
            reporter.logPass(message="The length and offset are appropriately set for empty metadata.")
        else:
            reporter.logError(message="The metadata offset (%d) and metadata length (%d) are not properly set. If one is 0, they both must be 0." % (metaOffset, metaLength))
            haveError = True
    # private data
    privOffset = header["privOffset"]
    privLength = header["privLength"]
    if privOffset == 0 or privLength == 0:
        if privOffset == 0 and privLength == 0:
            reporter.logPass(message="The length and offset are appropriately set for empty private data.")
        else:
            reporter.logError(message="The private data offset (%d) and private data length (%d) are not properly set. If one is 0, they both must be 0." % (privOffset, privLength))
            haveError = True
    return False, haveError

def _testBlocksPositioning(data, reporter):
    """
    Tests:
    - The table data must start immediately after the directory.
    - The table data must end at the beginning of the metadata, the beginning of the private data or the end of the file.
    - The metadata must start immediately after the table data.
    - the metadata must end at the beginning of he private data (padded as needed) or the end of the file.
    - The private data must start immediately after the table data or metadata.
    - The private data must end at the edge of the file.
    """
    header = unpackHeader(data)
    haveError = False
    # table data start
    directory = unpackDirectory(data)
    if not directory:
        return False, False
    expectedTableDataStart = headerSize + (directorySize * header["numTables"])
    offsets = [entry["offset"] for entry in directory]
    tableDataStart = min(offsets)
    if expectedTableDataStart != tableDataStart:
        reporter.logError(message="The table data does not start (%d) in the required position (%d)." % (tableDataStart, expectedTableDataStart))
        haveError = True
    else:
        reporter.logPass(message="The table data begins in the required position.")
    # table data end
    if header["metaOffset"]:
        definedTableDataEnd = header["metaOffset"]
    elif header["privOffset"]:
        definedTableDataEnd = header["privOffset"]
    else:
        definedTableDataEnd = header["length"]
    directory = unpackDirectory(data)
    ends = [table["offset"] + table["compLength"] + calcPaddingLength(table["compLength"]) for table in directory]
    expectedTableDataEnd = max(ends)
    if expectedTableDataEnd != definedTableDataEnd:
        reporter.logError(message="The table data end (%d) is not in the required position (%d)." % (definedTableDataEnd, expectedTableDataEnd))
        haveError = True
    else:
        reporter.logPass(message="The table data ends in the required position.")
    # metadata
    if header["metaOffset"]:
        # start
        expectedMetaStart = expectedTableDataEnd
        definedMetaStart = header["metaOffset"]
        if expectedMetaStart != definedMetaStart:
            reporter.logError(message="The metadata does not start (%d) in the required position (%d)." % (definedMetaStart, expectedMetaStart))
            haveError = True
        else:
            reporter.logPass(message="The metadata begins in the required position.")
        # end
        if header["privOffset"]:
            definedMetaEnd = header["privOffset"]
            needMetaPadding = True
        else:
            definedMetaEnd = header["length"]
            needMetaPadding = False
        expectedMetaEnd = header["metaOffset"] + header["metaLength"]
        if needMetaPadding:
            expectedMetaEnd += calcPaddingLength(header["metaLength"])
        if expectedMetaEnd != definedMetaEnd:
            reporter.logError(message="The metadata end (%d) is not in the required position (%d)." % (definedMetaEnd, expectedMetaEnd))
            haveError = True
        else:
            reporter.logPass(message="The metadata ends in the required position.")
    # private data
    if header["privOffset"]:
        # start
        if header["metaOffset"]:
            expectedPrivateStart = expectedMetaEnd
        else:
            expectedPrivateStart = expectedTableDataEnd
        definedPrivateStart = header["privOffset"]
        if expectedPrivateStart != definedPrivateStart:
            reporter.logError(message="The private data does not start (%d) in the required position (%d)." % (definedPrivateStart, expectedPrivateStart))
            haveError = True
        else:
            reporter.logPass(message="The private data begins in the required position.")
        # end
        expectedPrivateEnd = header["length"]
        definedPrivateEnd = header["privOffset"] + header["privLength"]
        if expectedPrivateEnd != definedPrivateEnd:
            reporter.logError(message="The private data end (%d) is not in the required position (%d)." % (definedPrivateEnd, expectedPrivateEnd))
            haveError = True
        else:
            reporter.logPass(message="The private data ends in the required position.")
    return False, haveError

# ----------------------
# Tests: Table Directory
# ----------------------

def testTableDirectory(data, reporter):
    """
    Test the WOFF table directory.
    """
    functions = [
        _testTableDirectoryStructure,
        _testTableDirectory4ByteOffsets,
        _testTableDirectoryPadding,
        _testTableDirectoryPositions,
        _testTableDirectoryCompressedLength,
        _testTableDirectoryDecompressedLength,
        _testTableDirectoryChecksums,
        _testTableDirectoryTableOrder
    ]
    nonStoppingError = False
    for function in functions:
        stoppingError, nsError = function(data, reporter)
        if nsError:
            nonStoppingError = True
        if stoppingError:
            return True, nonStoppingError
    return False, nonStoppingError

directoryFormat = """
    tag:            4s
    offset:         L
    compLength:     L
    origLength:     L
    origChecksum:   L
"""
directorySize = structCalcSize(directoryFormat)

def _testTableDirectoryStructure(data, reporter):
    """
    Tests:
    - The entries in the table directory can be unpacked.
    """
    header = unpackHeader(data)
    numTables = header["numTables"]
    data = data[headerSize:]
    try:
        for index in range(numTables):
            table, data = structUnpack(directoryFormat, data)
        reporter.logPass(message="The table directory structure is correct.")
    except:
        reporter.logError(message="The table directory is not properly structured.")
        return True, False
    return False, False

def _testTableDirectory4ByteOffsets(data, reporter):
    """
    Tests:
    - The font tables must each begin on a 4-byte boundary.
    """
    directory = unpackDirectory(data)
    haveError = False
    for table in directory:
        tag = table["tag"]
        offset = table["offset"]
        if offset % 4:
            reporter.logError(message="The \"%s\" table does not begin on a 4-byte boundary (%d)." % (tag, offset))
            haveError = True
        else:
            reporter.logPass(message="The \"%s\" table begins on a 4-byte boundary." % tag)
    return False, haveError

def _testTableDirectoryPadding(data, reporter):
    """
    Tests:
    - All tables, including the final table, must be padded to a
      four byte boundary using null bytes as needed.
    """
    header = unpackHeader(data)
    directory = unpackDirectory(data)
    haveError = False
    # test final table
    endError = False
    sfntEnd = None
    if header["metaOffset"] != 0:
        sfntEnd = header["metaOffset"]
    elif header["privOffset"] != 0:
        sfntEnd = header["privOffset"]
    else:
        sfntEnd = header["length"]
    if sfntEnd % 4:
        reporter.logError(message="The sfnt data does not end with proper padding.")
        haveError = True
    else:
        reporter.logPass(message="The sfnt data ends with proper padding.")
    # test the bytes used for padding
    for table in directory:
        tag = table["tag"]
        offset = table["offset"]
        length = table["compLength"]
        paddingLength = calcPaddingLength(length)
        if paddingLength:
            paddingOffset = offset + length
            padding = data[paddingOffset:paddingOffset+paddingLength]
            expectedPadding = b"\0" * paddingLength
            if padding != expectedPadding:
                reporter.logError(message="The \"%s\" table is not padded with null bytes." % tag)
                haveError = True
            else:
                reporter.logPass(message="The \"%s\" table is padded with null bytes." % tag)
    return False, haveError

def _testTableDirectoryPositions(data, reporter):
    """
    Tests:
    - The table offsets must not be before the end of the header/directory.
    - The table offset + length must not be greater than the edge of the available space.
    - The table offsets must not be after the edge of the available space.
    - Table blocks must not overlap.
    - There must be no gaps between the tables.
    """
    directory = unpackDirectory(data)
    tablesWithProblems = set()
    haveError = False
    # test for overlapping tables
    locations = []
    for table in directory:
        offset = table["offset"]
        length = table["compLength"]
        length = length + calcPaddingLength(length)
        locations.append((offset, offset + length, table["tag"]))
    for start, end, tag in locations:
        for otherStart, otherEnd, otherTag in locations:
            if tag == otherTag:
                continue
            if start >= otherStart and start < otherEnd:
                reporter.logError(message="The \"%s\" table overlaps the \"%s\" table." % (tag, otherTag))
                tablesWithProblems.add(tag)
                tablesWithProblems.add(otherTag)
                haveError = True
    # test for invalid offset, length and combo
    header = unpackHeader(data)
    if header["metaOffset"] != 0:
        tableDataEnd = header["metaOffset"]
    elif header["privOffset"] != 0:
        tableDataEnd = header["privOffset"]
    else:
        tableDataEnd = header["length"]
    numTables = header["numTables"]
    minOffset = headerSize + (directorySize * numTables)
    maxLength = tableDataEnd - minOffset
    for table in directory:
        tag = table["tag"]
        offset = table["offset"]
        length = table["compLength"]
        # offset is before the beginning of the table data block
        if offset < minOffset:
            tablesWithProblems.add(tag)
            message = "The \"%s\" table directory entry offset (%d) is before the start of the table data block (%d)." % (tag, offset, minOffset)
            reporter.logError(message=message)
            haveError = True
        # offset is after the end of the table data block
        elif offset > tableDataEnd:
            tablesWithProblems.add(tag)
            message = "The \"%s\" table directory entry offset (%d) is past the end of the table data block (%d)." % (tag, offset, tableDataEnd)
            reporter.logError(message=message)
            haveError = True
        # offset + length is after the end of the table tada block
        elif (offset + length) > tableDataEnd:
            tablesWithProblems.add(tag)
            message = "The \"%s\" table directory entry offset (%d) + length (%d) is past the end of the table data block (%d)." % (tag, offset, length, tableDataEnd)
            reporter.logError(message=message)
            haveError = True
    # test for gaps
    tables = []
    for table in directory:
        tag = table["tag"]
        offset = table["offset"]
        length = table["compLength"]
        length += calcPaddingLength(length)
        tables.append((offset, offset + length, tag))
    tables.sort()
    for index, (start, end, tag) in enumerate(tables):
        if index == 0:
            continue
        prevStart, prevEnd, prevTag = tables[index - 1]
        if prevEnd < start:
            tablesWithProblems.add(prevTag)
            tablesWithProblems.add(tag)
            reporter.logError(message="Extraneous data between the \"%s\" and \"%s\" tables." % (prevTag, tag))
            haveError = True
    # log passes
    for entry in directory:
        tag = entry["tag"]
        if tag in tablesWithProblems:
            continue
        reporter.logPass(message="The \"%s\" table directory entry has a valid offset and length." % tag)
    return False, haveError

def _testTableDirectoryCompressedLength(data, reporter):
    """
    Tests:
    - The compressed length must be less than or equal to the original length.
    """
    directory = unpackDirectory(data)
    haveError = False
    for table in directory:
        tag = table["tag"]
        compLength = table["compLength"]
        origLength = table["origLength"]
        if compLength > origLength:
            reporter.logError(message="The \"%s\" table directory entry has a compressed length (%d) larger than the original length (%d)." % (tag, compLength, origLength))
            haveError = True
        elif compLength == origLength:
            reporter.logPass(message="The \"%s\" table directory entry is not compressed." % tag)
        else:
            reporter.logPass(message="The \"%s\" table directory entry has proper compLength and origLength values." % tag)
    return False, haveError

def _testTableDirectoryDecompressedLength(data, reporter):
    """
    Tests:
    - The decompressed length of the data must match the defined original length.
    """
    directory = unpackDirectory(data)
    tableData = unpackTableData(data)
    haveError = False
    for table in directory:
        tag = table["tag"]
        offset = table["offset"]
        compLength = table["compLength"]
        origLength = table["origLength"]
        if compLength >= origLength:
            continue
        decompressedData = tableData[tag]
        # couldn't be decompressed. handled elsewhere.
        if decompressedData is None:
            continue
        decompressedLength = len(decompressedData)
        if origLength != decompressedLength:
            reporter.logError(message="The \"%s\" table directory entry has an original length (%d) that does not match the actual length of the decompressed data (%d)." % (tag, origLength, decompressedLength))
            haveError = True
        else:
            reporter.logPass(message="The \"%s\" table directory entry has a proper original length compared to the actual decompressed data." % tag)
    return False, haveError

def _testTableDirectoryChecksums(data, reporter):
    """
    Tests:
    - The checksums for the tables must match the checksums in the directory.
    - The head checksum adjustment must be correct.
    """
    # check the table directory checksums
    directory = unpackDirectory(data)
    tables = unpackTableData(data)
    haveError = False
    for entry in directory:
        tag = entry["tag"]
        origChecksum = entry["origChecksum"]
        decompressedData = tables[tag]
        # couldn't be decompressed.
        if decompressedData is None:
            continue
        newChecksum = calcChecksum(tag, decompressedData)
        if newChecksum != origChecksum:
            newChecksum = hex(newChecksum).strip("L")
            origChecksum = hex(origChecksum).strip("L")
            reporter.logError(message="The \"%s\" table directory entry original checksum (%s) does not match the checksum (%s) calculated from the data." % (tag, origChecksum, newChecksum))
            haveError = True
        else:
            reporter.logPass(message="The \"%s\" table directory entry original checksum is correct." % tag)
    # check the head checksum adjustment
    if b"head" not in tables:
        reporter.logWarning(message="The font does not contain a \"head\" table.")
    else:
        newChecksum = calcHeadChecksum(data)
        data = tables[b"head"]
        try:
            checksum = struct.unpack(">L", data[8:12])[0]
            if checksum != newChecksum:
                checksum = hex(checksum).strip("L")
                newChecksum = hex(newChecksum).strip("L")
                reporter.logError(message="The \"head\" table checkSumAdjustment (%s) does not match the calculated checkSumAdjustment (%s)." % (checksum, newChecksum))
                haveError = True
            else:
                reporter.logPass(message="The \"head\" table checkSumAdjustment is valid.")
        except:
            reporter.logError(message="The \"head\" table is not properly structured.")
            haveError = True
    return False, haveError

def _testTableDirectoryTableOrder(data, reporter):
    """
    Tests:
    - The directory entries must be stored in ascending order based on their tag.
    """
    storedOrder = [table["tag"] for table in unpackDirectory(data)]
    if storedOrder != sorted(storedOrder):
        reporter.logError(message="The table directory entries are not stored in alphabetical order.")
        return False, True
    else:
        reporter.logPass(message="The table directory entries are stored in the proper order.")
        return False, False

# -----------------
# Tests: Table Data
# -----------------

def testTableData(data, reporter):
    """
    Test the table data.
    """
    functions = [
        _testTableDataDecompression
    ]
    nonStoppingError = False
    for function in functions:
        stoppingError, nsError = function(data, reporter)
        if nsError:
            nonStoppingError = True
        if stoppingError:
            return True, nonStoppingError
    return False, nonStoppingError

def _testTableDataDecompression(data, reporter):
    """
    Tests:
    - The table data, when the defined compressed length is less
      than the original length, must be properly compressed.
    """
    haveError = False
    for table in unpackDirectory(data):
        tag = table["tag"]
        offset = table["offset"]
        compLength = table["compLength"]
        origLength = table["origLength"]
        if origLength <= compLength:
            continue
        entryData = data[offset:offset+compLength]
        try:
            decompressed = zlib.decompress(entryData)
            reporter.logPass(message="The \"%s\" table data can be decompressed with zlib." % tag)
        except zlib.error:
            reporter.logError(message="The \"%s\" table data can not be decompressed with zlib." % tag)
            haveError = True
    return False, haveError

# ----------------
# Tests: Metadata
# ----------------

def testMetadata(data, reporter):
    """
    Test the WOFF metadata.
    """
    if _shouldSkipMetadataTest(data, reporter):
        return False, False
    functions = [
        _testMetadataPadding,
        _testMetadataDecompression,
        _testMetadataDecompressedLength,
        _testMetadataParse,
        _testMetadataEncoding,
        _testMetadataStructure
    ]
    nonStoppingError = False
    for function in functions:
        stoppingError, nsError = function(data, reporter)
        if nsError:
            nonStoppingError = True
        if stoppingError:
            return True, nonStoppingError
    return False, nonStoppingError

def _shouldSkipMetadataTest(data, reporter):
    """
    This is used at the start of metadata test functions.
    It writes a note and returns True if not metadata exists.
    """
    header = unpackHeader(data)
    metaOffset = header["metaOffset"]
    metaLength = header["metaLength"]
    if metaOffset == 0 or metaLength == 0:
        reporter.logNote(message="No metadata to test.")
        return True

def _testMetadataPadding(data, reporter):
    """
    - The padding must be null.
    """
    header = unpackHeader(data)
    if not header["metaOffset"] or not header["privOffset"]:
        return False, False
    paddingLength = calcPaddingLength(header["metaLength"])
    if not paddingLength:
        return False, False
    paddingOffset = header["metaOffset"] + header["metaLength"]
    padding = data[paddingOffset:paddingOffset + paddingLength]
    expectedPadding = "\0" * paddingLength
    if padding != expectedPadding:
        reporter.logError(message="The metadata is not padded with null bytes.")
        return False, True
    else:
        reporter.logPass(message="The metadata is padded with null bytes,")
        return False, False

# does this need to be tested?
#
# def testMetadataIsCompressed(data, reporter):
#     """
#     Tests:
#     - The metadata must be compressed.
#     """
#     if _shouldSkipMetadataTest(data, reporter):
#         return
#     header = unpackHeader(data)
#     length = header["metaLength"]
#     origLength = header["metaOrigLength"]
#     if length >= origLength:
#         reporter.logError(message="The compressed metdata length (%d) is higher than or equal to the original, uncompressed length (%d)." % (length, origLength))
#         return True
#     reporter.logPass(message="The compressed metdata length is smaller than the original, uncompressed length.")

def _testMetadataDecompression(data, reporter):
    """
    Tests:
    - Metadata must be compressed with zlib.
    """
    if _shouldSkipMetadataTest(data, reporter):
        return False, False
    compData = unpackMetadata(data, decompress=False, parse=False)
    try:
        zlib.decompress(compData)
    except zlib.error:
        reporter.logError(message="The metadata can not be decompressed with zlib.")
        return True, False
    reporter.logPass(message="The metadata can be decompressed with zlib.")
    return False, False

def _testMetadataDecompressedLength(data, reporter):
    """
    Tests:
    - The length of the decompressed metadata must match the defined original length.
    """
    if _shouldSkipMetadataTest(data, reporter):
        return False, False
    header = unpackHeader(data)
    metadata = unpackMetadata(data, parse=False)
    metaOrigLength = header["metaOrigLength"]
    decompressedLength = len(metadata)
    if metaOrigLength != decompressedLength:
        reporter.logError(message="The decompressed metadata length (%d) does not match the original metadata length (%d) in the header." % (decompressedLength, metaOrigLength))
        return False, True
    else:
        reporter.logPass(message="The decompressed metadata length matches the original metadata length in the header.")
        return False, False

def _testMetadataParse(data, reporter):
    """
    Tests:
    - The metadata must be well-formed.
    """
    if _shouldSkipMetadataTest(data, reporter):
        return False, False
    metadata = unpackMetadata(data, parse=False)
    try:
        tree = ElementTree.fromstring(metadata)
    except (ExpatError, LookupError):
        reporter.logError(message="The metadata can not be parsed.")
        return True, False
    reporter.logPass(message="The metadata can be parsed.")
    return False, False

def _testMetadataEncoding(data, reporter):
    """
    Tests:
    - The metadata must be UTF-8 encoded.
    """
    if _shouldSkipMetadataTest(data, reporter):
        return False, False
    metadata = unpackMetadata(data, parse=False)
    errorMessage = "The metadata encoding is not valid."
    encoding = None
    # check the BOM
    if not metadata.startswith("<"):
        if not metadata.startswith(codecs.BOM_UTF8):
            reporter.logError(message=errorMessage)
            return False, True
        else:
            encoding = "UTF-8"
    # sniff the encoding
    else:
        # quick test to ensure that the regular expression will work.
        # the string must start with <?xml. this will catch
        # other encodings such as: <\x00?\x00x\x00m\x00l
        if not metadata.startswith("<?xml"):
            reporter.logError(message=errorMessage)
            return False, True
        # go to the first occurance of >
        line = metadata.split(">", 1)[0]
        # find an encoding string
        pattern = re.compile(
            "\s+"
            "encoding"
            "\s*"
            "="
            "\s*"
            "[\"']+"
            "([^\"']+)"
        )
        m = pattern.search(line)
        if m:
            encoding = m.group(1)
        else:
            encoding = "UTF-8"
    # report
    if encoding != "UTF-8":
        reporter.logError(message=errorMessage)
        return False, True
    else:
        reporter.logPass(message="The metadata is properly encoded.")
        return False, False

def _testMetadataStructure(data, reporter):
    """
    Test the metadata structure.
    """
    if _shouldSkipMetadataTest(data, reporter):
        return False, False
    tree = unpackMetadata(data)
    # make sure the top element is metadata
    if tree.tag != "metadata":
        reporter.logError("The top element is not \"metadata\".")
        return False, True
    # sniff the version
    version = tree.attrib.get("version")
    if not version:
        reporter.logError("The \"version\" attribute is not defined.")
        return False, True
    # grab the appropriate specification
    versionSpecs = {
        "1.0" : metadataSpec_1_0
    }
    spec = versionSpecs.get(version)
    if spec is None:
        reporter.logError("Unknown version (\"%s\")." % version)
        return False, True
    haveError = _validateMetadataElement(tree, spec, reporter)
    if not haveError:
        reporter.logPass("The \"metadata\" element is properly formatted.")
    return False, haveError

def _validateMetadataElement(element, spec, reporter, parentTree=[]):
    haveError = False
    # unknown attributes
    knownAttributes = []
    for attrib in spec["requiredAttributes"].keys() + spec["recommendedAttributes"].keys() + spec["optionalAttributes"].keys():
        attrib = _parseAttribute(attrib)
        knownAttributes.append(attrib)
    for attrib in sorted(element.attrib.keys()):
        # the search is a bit complicated because there are
        # attributes that have more than one name.
        found = False
        for knownAttrib in knownAttributes:
            if knownAttrib == attrib:
                found = True
                break
            elif isinstance(knownAttrib, list) and attrib in knownAttrib:
                found = True
                break
        if not found:
            _logMetadataResult(
                reporter,
                "error",
                "Unknown attribute (\"%s\")" % attrib,
                element.tag,
                parentTree
            )
            haveError = True
    # attributes
    s = [
        ("requiredAttributes", "required"),
        ("recommendedAttributes", "recommended"),
        ("optionalAttributes", "optional")
    ]
    for key, requirementLevel in s:
        if spec[key]:
            e = _validateAttributes(element, spec[key], reporter, parentTree, requirementLevel)
            if e:
                haveError = True
    # unknown child-elements
    knownChildElements = spec["requiredChildElements"].keys() + spec["recommendedChildElements"].keys() + spec["optionalChildElements"].keys()
    for childElement in element:
        if childElement.tag not in knownChildElements:
           _logMetadataResult(
               reporter,
               "error",
               "Unknown child-element (\"%s\")" % childElement.tag,
               element.tag,
               parentTree
           )
           haveError = True
    # child elements
    s = [
        ("requiredChildElements", "required"),
        ("recommendedChildElements", "recommended"),
        ("optionalChildElements", "optional")
    ]
    for key, requirementLevel in s:
        if spec[key]:
            for childElementTag, childElementData in sorted(spec[key].items()):
                e = _validateChildElements(element, childElementTag, childElementData, reporter, parentTree, requirementLevel)
                if e:
                    haveError = True
    # content
    content = element.text
    if content is not None:
        content = content.strip()
    if content and spec["content"] == "not allowed":
        _logMetadataResult(
            reporter,
            "error",
            "Content defined",
            element.tag,
            parentTree
        )
        haveError = True
    elif not content and content and spec["content"] == "required":
        _logMetadataResult(
            reporter,
            "error",
            "Content not defined",
            element.tag,
            parentTree
        )
    elif not content and spec["content"] == "recommended":
        _logMetadataResult(
            reporter,
            "warn",
            "Content not defined",
            element.tag,
            parentTree
        )
    # log the result
    if not haveError and parentTree == ["metadata"]:
        reporter.logPass("The \"%s\" element is properly formatted." % element.tag)
    # done
    return haveError

def _parseAttribute(attrib):
    if " " in attrib:
        final = []
        for a in attrib.split(" "):
            if a.startswith("xml:"):
                a = "{http://www.w3.org/XML/1998/namespace}" + a[4:]
            final.append(a)
        return final
    return attrib

def _unEtreeAttribute(attrib):
    ns = "{http://www.w3.org/XML/1998/namespace}"
    if attrib.startswith(ns):
        attrib = "xml:" + attrib[len(ns):]
    return attrib

def _validateAttributes(element, spec, reporter, parentTree, requirementLevel):
    haveError = False
    for attrib, valueOptions in sorted(spec.items()):
        attribs = _parseAttribute(attrib)
        if isinstance(attribs, basestring):
            attribs = [attribs]
        found = []
        for attrib in attribs:
            if attrib in element.attrib:
                found.append(attrib)
        # make strings for reporting
        if len(attribs) > 1:
            attribString = ", ".join(["\"%s\"" % _unEtreeAttribute(i) for i in attribs])
        else:
            attribString = "\"%s\"" % attribs[0]
        if len(found) == 0:
            pass
        elif len(found) > 1:
            foundString = ", ".join(["\"%s\"" % _unEtreeAttribute(i) for i in found])
        else:
            foundString = "\"%s\"" % found[0]
        # more than one of the mutually exclusive attributes found
        if len(found) > 1:
            _logMetadataResult(
                reporter,
                "error",
                "More than one mutually exclusive attribute (%s) defined" % foundString,
                element.tag,
                parentTree
            )
            haveError = True
        # missing
        elif len(found) == 0:
            if requirementLevel == "optional":
                continue
            elif requirementLevel == "required":
                errorLevel = "error"
            else:
                errorLevel = "warn"
            _logMetadataResult(
                reporter,
                errorLevel,
                "%s \"%s\" attribute not defined" % (requirementLevel.title(), attrib),
                element.tag,
                parentTree
            )
            if requirementLevel == "required":
                haveError = True
        # incorrect value
        else:
            e = _validateAttributeValue(element, found[0], valueOptions, reporter, parentTree)
            if e:
                haveError = True
    # done
    return haveError

def _validateAttributeValue(element, attrib, valueOptions, reporter, parentTree):
    haveError = False
    value = element.attrib[attrib]
    if isinstance(valueOptions, basestring):
        valueOptions = [valueOptions]
    # no defined value options
    if valueOptions is None:
        # the string is empty
        if not value:
            _logMetadataResult(
                reporter,
                "warn",
                "Value for the \"%s\" attribute is an empty string" % attrib,
                element.tag,
                parentTree
            )
    # illegal value
    elif value not in valueOptions:
        _logMetadataResult(
            reporter,
            "error",
            "Invalid value (\"%s\") for the \"%s\" attribute" % (value, attrib),
            element.tag,
            parentTree
        )
        haveError = True
    # return the error state
    return haveError

def _validateChildElements(element, childElementTag, childElementData, reporter, parentTree, requirementLevel):
    haveError = False
    # get the valid counts
    minimumOccurrences = childElementData.get("minimumOccurrences", 0)
    maximumOccurrences = childElementData.get("maximumOccurrences", None)
    # find the appropriate elements
    found = element.findall(childElementTag)
    # not defined enough times
    if minimumOccurrences == 1 and len(found) == 0:
        _logMetadataResult(
            reporter,
            "error",
            "%s \"%s\" child-element not defined" % (requirementLevel.title(), childElementTag),
            element.tag,
            parentTree
        )
        haveError = True
    elif len(found) < minimumOccurrences:
        _logMetadataResult(
            reporter,
            "error",
            "%s \"%s\" child-element is defined %d times instead of the minimum %d times" % (requirementLevel.title(), childElementTag, len(found), minimumOccurrences),
            element.tag,
            parentTree
        )
        haveError = True
    # not defined, but not recommended
    elif len(found) == 0 and requirementLevel == "recommended":
        _logMetadataResult(
            reporter,
            "warn",
            "%s \"%s\" child-element is not defined" % (requirementLevel.title(), childElementTag),
            element.tag,
            parentTree
        )
    # defined too many times
    if maximumOccurrences is not None:
        if maximumOccurrences == 1 and len(found) > 1:
            _logMetadataResult(
                reporter,
                "error",
                "%s \"%s\" child-element defined more than once" % (requirementLevel.title(), childElementTag),
                element.tag,
                parentTree
            )
            haveError = True
        elif len(found) > maximumOccurrences:
            _logMetadataResult(
                reporter,
                "error",
                "%s \"%s\" child-element defined %d times instead of the maximum %d times" % (requirementLevel.title(), childElementTag, len(found), minimumOccurrences),
                element.tag,
                parentTree
            )
            haveError = True
    # validate the found elements
    if not haveError:
        for childElement in found:
            # handle recursive child-elements
            childElementSpec = childElementData["spec"]
            if childElementSpec == "recursive divSpec_1_0":
                childElementSpec = divSpec_1_0
            elif childElementSpec == "recursive spanSpec_1_0":
                childElementSpec = spanSpec_1_0
            # dive
            e = _validateMetadataElement(childElement, childElementSpec, reporter, parentTree + [element.tag])
            if e:
                haveError = True
    # return the error state
    return haveError

# logging support

def _logMetadataResult(reporter, result, message, elementTag, parentTree):
    message = _formatMetadataResultMessage(message, elementTag, parentTree)
    methods = {
        "error" : reporter.logError,
        "warn" : reporter.logWarning,
        "note" : reporter.logNote,
        "pass" : reporter.logPass
    }
    methods[result](message)

def _formatMetadataResultMessage(message, elementTag, parentTree):
    parentTree = parentTree + [elementTag]
    if parentTree[0] == "metadata":
        parentTree = parentTree[1:]
    if parentTree:
        parentTree = ["\"%s\"" % t for t in reversed(parentTree) if t is not None]
        message += " in " + " in ".join(parentTree)
    message += "."
    return message

# ----------------
# Metadata Display
# ----------------

def getMetadataForDisplay(data):
    """
    Build a tree of the metadata. The value returned will
    be a list of elements in the following dict form:

        {
            tag : {
                attributes : {
                    attribute : value
                },
            text : string,
            children : []
            }
        }

    The value for "children" will be a list of elements
    folowing the same structure defined above.
    """
    test = unpackMetadata(data, parse=False)
    if not test:
        return None
    metadata = unpackMetadata(data)
    tree = []
    for element in metadata:
        _recurseMetadataElement(element, tree)
    return tree

def _recurseMetadataElement(element, tree):
    # tag
    tag = element.tag
    # text
    text = element.text
    if text:
        text = text.strip()
    if not text:
        text = None
    # attributes
    attributes = {}
    ns = "{http://www.w3.org/XML/1998/namespace}"
    for key, value in element.attrib.items():
        if key.startswith(ns):
            key = "xml:" + key[len(ns):]
        attributes[key] = value
    # compile the dictionary
    d = dict(
        tag=tag,
        attributes=attributes,
        text=text,
        children=[]
    )
    tree.append(d)
    # run through the children
    for sub in element:
        _recurseMetadataElement(sub, d["children"])


# -------------------------
# Support: Misc. SFNT Stuff
# -------------------------

# Some of this was adapted from fontTools.ttLib.sfnt

sfntHeaderFormat = """
    sfntVersion:    4s
    numTables:      H
    searchRange:    H
    entrySelector:  H
    rangeShift:     H
"""
sfntHeaderSize = structCalcSize(sfntHeaderFormat)

sfntDirectoryEntryFormat = """
    tag:            4s
    checkSum:       L
    offset:         L
    length:         L
"""
sfntDirectoryEntrySize = structCalcSize(sfntDirectoryEntryFormat)

def maxPowerOfTwo(value):
    exponent = 0
    while value:
        value = value >> 1
        exponent += 1
    return max(exponent - 1, 0)

def getSearchRange(numTables):
    exponent = maxPowerOfTwo(numTables)
    searchRange = (2 ** exponent) * 16
    entrySelector = exponent
    rangeShift = numTables * 16 - searchRange
    return searchRange, entrySelector, rangeShift

def calcPaddingLength(length):
    if not length % 4:
        return 0
    return 4 - (length % 4)

def padData(data):
    data += b"\0" * calcPaddingLength(len(data))
    return data

def sumDataULongs(data):
    longs = struct.unpack(">%dL" % (len(data) / 4), data)
    value = sum(longs) % (2 ** 32)
    return value

def calcChecksum(tag, data):
    if tag == b"head":
        data = data[:8] + b"\0\0\0\0" + data[12:]
    data = padData(data)
    value = sumDataULongs(data)
    return value

def calcHeadChecksum(data):
    header = unpackHeader(data)
    directory = unpackDirectory(data)
    numTables = header["numTables"]
    # build the sfnt directory
    searchRange, entrySelector, rangeShift = getSearchRange(numTables)
    sfntHeaderData = dict(
        sfntVersion=header["flavor"],
        numTables=numTables,
        searchRange=searchRange,
        entrySelector=entrySelector,
        rangeShift=rangeShift
    )
    sfntData = structPack(sfntHeaderFormat, sfntHeaderData)
    sfntEntries = {}
    offset = sfntHeaderSize + (sfntDirectoryEntrySize * numTables)
    directory = [(entry["offset"], entry) for entry in directory]
    for o, entry in sorted(directory):
        checksum = entry["origChecksum"]
        tag = entry["tag"]
        length = entry["origLength"]
        sfntEntries[tag] = dict(
            tag=tag,
            checkSum=checksum,
            offset=offset,
            length=length
        )
        offset += length + calcPaddingLength(length)
    for tag, sfntEntry in sorted(sfntEntries.items()):
        sfntData += structPack(sfntDirectoryEntryFormat, sfntEntry)
    # calculate
    checkSums = [entry["checkSum"] for entry in sfntEntries.values()]
    checkSums.append(sumDataULongs(sfntData))
    checkSum = sum(checkSums)
    checkSum = (0xB1B0AFBA - checkSum) & 0xffffffff
    return checkSum

# ------------------
# Support XML Writer
# ------------------

class XMLWriter(object):

    def __init__(self):
        self._root = None
        self._elements = []

    def simpletag(self, tag, **kwargs):
        ElementTree.SubElement(self._elements[-1], tag, **kwargs)

    def begintag(self, tag, **kwargs):
        if self._elements:
            s = ElementTree.SubElement(self._elements[-1], tag, **kwargs)
        else:
            s = ElementTree.Element(tag, **kwargs)
            if self._root is None:
                self._root = s
        self._elements.append(s)

    def endtag(self, tag):
        assert self._elements[-1].tag == tag
        del self._elements[-1]

    def write(self, text):
        if self._elements[-1].text is None:
            self._elements[-1].text = text
        else:
            self._elements[-1].text += text

    def compile(self, encoding="utf-8"):
        f = StringIO()
        tree = ElementTree.ElementTree(self._root)
        indent(tree.getroot())
        tree.write(f, encoding=encoding)
        text = f.getvalue()
        del f
        return text

def indent(elem, level=0):
    # this is from http://effbot.python-hosting.com/file/effbotlib/ElementTree.py
    i = "\n" + level * "\t"
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "\t"
        for e in elem:
            indent(e, level + 1)
        if not e.tail or not e.tail.strip():
            e.tail = i
    if level and (not elem.tail or not elem.tail.strip()):
        elem.tail = i

# ---------------------------------
# Support: Reporters and HTML Stuff
# ---------------------------------

class TestResultGroup(list):

    def __init__(self, title):
        super(TestResultGroup, self).__init__()
        self.title = title

    def _haveType(self, tp):
        for data in self:
            if data["type"] == tp:
                return True
        return False

    def haveNote(self):
        return self._haveType("NOTE")

    def haveWarning(self):
        return self._haveType("WARNING")

    def haveError(self):
        return self._haveType("ERROR")

    def havePass(self):
        return self._haveType("PASS")

    def haveTraceback(self):
        return self._haveType("TRACEBACK")


class BaseReporter(object):

    """
    Base reporter. This establishes the required API for reporters.
    """

    def __init__(self):
        self.title = ""
        self.fileInfo = []
        self.metadata = None
        self.testResults = []
        self.haveReadError = False

    def logTitle(self, title):
        self.title = title

    def logFileInfo(self, title, value):
        self.fileInfo.append((title, value))

    def logMetadata(self, data):
        self.metadata = data

    def logTableInfo(self, tag=None, offset=None, compLength=None, origLength=None, origChecksum=None):
        self.tableInfo.append((tag, offset, compLength, origLength, origChecksum))

    def logTestTitle(self, title):
        self.testResults.append(TestResultGroup(title))

    def logNote(self, message, information=""):
        d = dict(type="NOTE", message=message, information=information)
        self.testResults[-1].append(d)

    def logWarning(self, message, information=""):
        d = dict(type="WARNING", message=message, information=information)
        self.testResults[-1].append(d)

    def logError(self, message, information=""):
        d = dict(type="ERROR", message=message, information=information)
        self.testResults[-1].append(d)

    def logPass(self, message, information=""):
        d = dict(type="PASS", message=message, information=information)
        self.testResults[-1].append(d)

    def logTraceback(self, text):
        d = dict(type="TRACEBACK", message=text, information="")
        self.testResults[-1].append(d)

    def getReport(self, *args, **kwargs):
        raise NotImplementedError


class TextReporter(BaseReporter):

    """
    Plain text reporter.
    """

    def getReport(self, reportNote=True, reportWarning=True, reportError=True, reportPass=True):
        report = []
        if self.metadata is not None:
            report.append("METADATA DISPLAY")
        for group in self.testResults:
            for result in group:
                typ = result["type"]
                if typ == "NOTE" and not reportNote:
                    continue
                elif typ == "WARNING" and not reportWarning:
                    continue
                elif typ == "ERROR" and not reportError:
                    continue
                elif typ == "PASS" and not reportPass:
                    continue
                t = "%s - %s: %s" % (result["type"], group.title, result["message"])
                report.append(t)
        return "\n".join(report)


class HTMLReporter(BaseReporter):

    def getReport(self):
        writer = startHTML(title=self.title)
        # write the file info
        self._writeFileInfo(writer)
        # write major error alert
        if self.haveReadError:
            self._writeMajorError(writer)
        # write the metadata
        if self.metadata:
            self._writeMetadata(writer)
        # write the test overview
        self._writeTestResultsOverview(writer)
        # write the test groups
        self._writeTestResults(writer)
        # close the html
        text = finishHTML(writer)
        # done
        return text

    def _writeFileInfo(self, writer):
        # write the font info
        writer.begintag("div", c_l_a_s_s="infoBlock")
        ## title
        writer.begintag("h3", c_l_a_s_s="infoBlockTitle")
        writer.write("File Information")
        writer.endtag("h3")
        ## table
        writer.begintag("table", c_l_a_s_s="report")
        ## items
        for title, value in self.fileInfo:
            # row
            writer.begintag("tr")
            # title
            writer.begintag("td", c_l_a_s_s="title")
            writer.write(title)
            writer.endtag("td")
            # message
            writer.begintag("td")
            writer.write(value)
            writer.endtag("td")
            # close row
            writer.endtag("tr")
        writer.endtag("table")
        ## close the container
        writer.endtag("div")

    def _writeMajorError(self, writer):
        writer.begintag("h2", c_l_a_s_s="readError")
        writer.write("The file contains major structural errors!")
        writer.endtag("h2")

    def _writeMetadata(self, writer):
        # start the block
        writer.begintag("div", c_l_a_s_s="infoBlock")
        # title
        writer.begintag("h3", c_l_a_s_s="infoBlockTitle")
        writer.write("Metadata ")
        writer.endtag("h3")
        # content
        for element in self.metadata:
            self._writeMetadataElement(element, writer)
        # close the block
        writer.endtag("div")

    def _writeMetadataElement(self, element, writer):
        writer.begintag("div", c_l_a_s_s="metadataElement")
        # tag
        writer.begintag("h5", c_l_a_s_s="metadata")
        writer.write(element["tag"])
        writer.endtag("h5")
        # attributes
        attributes = element["attributes"]
        if len(attributes):
            writer.begintag("h6", c_l_a_s_s="metadata")
            writer.write("Attributes:")
            writer.endtag("h6")
            # key, value pairs
            writer.begintag("table", c_l_a_s_s="metadata")
            for key, value in sorted(attributes.items()):
                writer.begintag("tr")
                writer.begintag("td", c_l_a_s_s="key")
                writer.write(key)
                writer.endtag("td")
                writer.begintag("td", c_l_a_s_s="value")
                writer.write(value)
                writer.endtag("td")
                writer.endtag("tr")
            writer.endtag("table")
        # text
        text = element["text"]
        if text is not None and text.strip():
            writer.begintag("h6", c_l_a_s_s="metadata")
            writer.write("Text:")
            writer.endtag("h6")
            writer.begintag("p", c_l_a_s_s="metadata")
            writer.write(text)
            writer.endtag("p")
        # child elements
        children = element["children"]
        if len(children):
            writer.begintag("h6", c_l_a_s_s="metadata")
            writer.write("Child Elements:")
            writer.endtag("h6")
            for child in children:
                self._writeMetadataElement(child, writer)
        # close
        writer.endtag("div")

    def _writeTestResultsOverview(self, writer):
        ## tabulate
        notes = 0
        passes = 0
        errors = 0
        warnings = 0
        for group in self.testResults:
            for data in group:
                tp = data["type"]
                if tp == "NOTE":
                    notes += 1
                elif tp == "PASS":
                    passes += 1
                elif tp == "ERROR":
                    errors += 1
                else:
                    warnings += 1
        total = sum((notes, passes, errors, warnings))
        ## container
        writer.begintag("div", c_l_a_s_s="infoBlock")
        ## header
        writer.begintag("h3", c_l_a_s_s="infoBlockTitle")
        writer.write("Results for %d Tests" % total)
        writer.endtag("h3")
        ## results
        results = [
            ("PASS", passes),
            ("WARNING", warnings),
            ("ERROR", errors),
            ("NOTE", notes),
        ]
        writer.begintag("table", c_l_a_s_s="report")
        for tp, value in results:
            # title
            writer.begintag("tr", c_l_a_s_s="testReport%s" % tp.title())
            writer.begintag("td", c_l_a_s_s="title")
            writer.write(tp)
            writer.endtag("td")
            # count
            writer.begintag("td", c_l_a_s_s="testReportResultCount")
            writer.write(str(value))
            writer.endtag("td")
            # empty
            writer.begintag("td")
            writer.endtag("td")
            # toggle button
            buttonID = "testResult%sToggleButton" % tp
            writer.begintag("td",
                id=buttonID, c_l_a_s_s="toggleButton",
                onclick="testResultToggleButtonHit(a_p_o_s_t_r_o_p_h_e%sa_p_o_s_t_r_o_p_h_e, a_p_o_s_t_r_o_p_h_e%sa_p_o_s_t_r_o_p_h_e);" % (buttonID, "test%s" % tp.title()))
            writer.write("Hide")
            writer.endtag("td")
            # close the row
            writer.endtag("tr")
        writer.endtag("table")
        ## close the container
        writer.endtag("div")

    def _writeTestResults(self, writer):
        for infoBlock in self.testResults:
            # container
            writer.begintag("div", c_l_a_s_s="infoBlock")
            # header
            writer.begintag("h4", c_l_a_s_s="infoBlockTitle")
            writer.write(infoBlock.title)
            writer.endtag("h4")
            # individual reports
            writer.begintag("table", c_l_a_s_s="report")
            for data in infoBlock:
                tp = data["type"]
                message = data["message"]
                information = data["information"]
                # row
                writer.begintag("tr", c_l_a_s_s="test%s" % tp.title())
                # title
                writer.begintag("td", c_l_a_s_s="title")
                writer.write(tp)
                writer.endtag("td")
                # message
                writer.begintag("td")
                writer.write(message)
                ## info
                if information:
                    writer.begintag("p", c_l_a_s_s="info")
                    writer.write(information)
                    writer.endtag("p")
                writer.endtag("td")
                # close row
                writer.endtag("tr")
            writer.endtag("table")
            # close container
            writer.endtag("div")


defaultCSS = """
body {
	background-color: #e5e5e5;
	padding: 15px 15px 0px 15px;
	margin: 0px;
	font-family: Helvetica, Verdana, Arial, sans-serif;
}

h2.readError {
	background-color: red;
	color: white;
	margin: 20px 15px 20px 15px;
	padding: 10px;
	border-radius: 5px;
	font-size: 25px;
}

/* info blocks */

.infoBlock {
	background-color: white;
	margin: 0px 0px 15px 0px;
	padding: 15px;
	border-radius: 5px;
}

h3.infoBlockTitle {
	font-size: 20px;
	margin: 0px 0px 15px 0px;
	padding: 0px 0px 10px 0px;
	border-bottom: 1px solid #e5e5e5;
}

h4.infoBlockTitle {
	font-size: 17px;
	margin: 0px 0px 15px 0px;
	padding: 0px 0px 10px 0px;
	border-bottom: 1px solid #e5e5e5;
}

table.report {
	border-collapse: collapse;
	width: 100%;
	font-size: 14px;
}

table.report tr {
	border-top: 1px solid white;
}

table.report tr.testPass, table.report tr.testReportPass {
	background-color: #c8ffaf;
}

table.report tr.testError, table.report tr.testReportError {
	background-color: #ffc3af;
}

table.report tr.testWarning, table.report tr.testReportWarning {
	background-color: #ffe1af;
}

table.report tr.testNote, table.report tr.testReportNote {
	background-color: #96e1ff;
}

table.report tr.testTraceback, table.report tr.testReportTraceback {
	background-color: red;
	color: white;
}

table.report td {
	padding: 7px 5px 7px 5px;
	vertical-align: top;
}

table.report td.title {
	width: 80px;
	text-align: right;
	font-weight: bold;
	text-transform: uppercase;
}

table.report td.testReportResultCount {
	width: 100px;
}

table.report td.toggleButton {
	text-align: center;
	width: 50px;
	border-left: 1px solid white;
	cursor: pointer;
}

.infoBlock td p.info {
	font-size: 12px;
	font-style: italic;
	margin: 5px 0px 0px 0px;
}

/* SFNT table */

table.sfntTableData {
	font-size: 14px;
	width: 100%;
	border-collapse: collapse;
	padding: 0px;
}

table.sfntTableData th {
	padding: 5px 0px 5px 0px;
	text-align: left
}

table.sfntTableData tr.uncompressed {
	background-color: #ffc3af;
}

table.sfntTableData td {
	width: 20%;
	padding: 5px 0px 5px 0px;
	border: 1px solid #e5e5e5;
	border-left: none;
	border-right: none;
	font-family: Consolas, Menlo, "Vera Mono", Monaco, monospace;
}

pre {
	font-size: 12px;
	font-family: Consolas, Menlo, "Vera Mono", Monaco, monospace;
	margin: 0px;
	padding: 0px;
}

/* Metadata */

.metadataElement {
	background: rgba(0, 0, 0, 0.03);
	margin: 10px 0px 10px 0px;
	border: 2px solid #d8d8d8;
	padding: 10px;
}

h5.metadata {
	font-size: 14px;
	margin: 5px 0px 10px 0px;
	padding: 0px 0px 5px 0px;
	border-bottom: 1px solid #d8d8d8;
}

h6.metadata {
	font-size: 12px;
	font-weight: normal;
	margin: 10px 0px 10px 0px;
	padding: 0px 0px 5px 0px;
	border-bottom: 1px solid #d8d8d8;
}

table.metadata {
	font-size: 12px;
	width: 100%;
	border-collapse: collapse;
	padding: 0px;
}

table.metadata td.key {
	width: 5em;
	padding: 5px 5px 5px 0px;
	border-right: 1px solid #d8d8d8;
	text-align: right;
	vertical-align: top;
}

table.metadata td.value {
	padding: 5px 0px 5px 5px;
	border-left: 1px solid #d8d8d8;
	text-align: left;
	vertical-align: top;
}

p.metadata {
	font-size: 12px;
	font-style: italic;
}
}
"""

defaultJavascript = """

//<![CDATA[
	function testResultToggleButtonHit(buttonID, className) {
		// change the button title
		var element = document.getElementById(buttonID);
		if (element.innerHTML == "Show" ) {
			element.innerHTML = "Hide";
		}
		else {
			element.innerHTML = "Show";
		}
		// toggle the elements
		var elements = getTestResults(className);
		for (var e = 0; e < elements.length; ++e) {
			toggleElement(elements[e]);
		}
		// toggle the info blocks
		toggleInfoBlocks();
	}

	function getTestResults(className) {
		var rows = document.getElementsByTagName("tr");
		var found = Array();
		for (var r = 0; r < rows.length; ++r) {
			var row = rows[r];
			if (row.className == className) {
				found[found.length] = row;
			}
		}
		return found;
	}

	function toggleElement(element) {
		if (element.style.display != "none" ) {
			element.style.display = "none";
		}
		else {
			element.style.display = "";
		}
	}

	function toggleInfoBlocks() {
		var tables = document.getElementsByTagName("table")
		for (var t = 0; t < tables.length; ++t) {
			var table = tables[t];
			if (table.className == "report") {
				var haveVisibleRow = false;
				var rows = table.rows;
				for (var r = 0; r < rows.length; ++r) {
					var row = rows[r];
					if (row.style.display == "none") {
						var i = 0;
					}
					else {
						haveVisibleRow = true;
					}
				}
				var div = table.parentNode;
				if (haveVisibleRow == true) {
					div.style.display = "";
				}
				else {
					div.style.display = "none";
				}
			}
		}
	}
//]]>
"""

def startHTML(title=None, cssReplacements={}):
    writer = XMLWriter()
    # start the html
    writer.begintag("html", xmlns="http://www.w3.org/1999/xhtml", lang="en")
    # start the head
    writer.begintag("head")
    writer.simpletag("meta", http_equiv="Content-Type", content="text/html; charset=utf-8")
    # title
    if title is not None:
        writer.begintag("title")
        writer.write(title)
        writer.endtag("title")
    # write the css
    writer.begintag("style", type="text/css")
    css = defaultCSS
    for before, after in cssReplacements.items():
        css = css.replace(before, after)
    writer.write(css)
    writer.endtag("style")
    # write the javascript
    writer.begintag("script", type="text/javascript")
    javascript = defaultJavascript
    ## hack around some ElementTree escaping
    javascript = javascript.replace("<", "l_e_s_s")
    javascript = javascript.replace(">", "g_r_e_a_t_e_r")
    writer.write(javascript)
    writer.endtag("script")
    # close the head
    writer.endtag("head")
    # start the body
    writer.begintag("body")
    # return the writer
    return writer

def finishHTML(writer):
    # close the body
    writer.endtag("body")
    # close the html
    writer.endtag("html")
    # get the text
    text = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
    text += writer.compile()
    text = text.replace("c_l_a_s_s", "class")
    text = text.replace("a_p_o_s_t_r_o_p_h_e", "'")
    text = text.replace("l_e_s_s", "<")
    text = text.replace("g_r_e_a_t_e_r", ">")
    text = text.replace("http_equiv", "http-equiv")
    # return
    return text

# ------------------
# Support: Unpackers
# ------------------

def unpackHeader(data):
    return structUnpack(headerFormat, data)[0]

def unpackDirectory(data):
    header = unpackHeader(data)
    numTables = header["numTables"]
    data = data[headerSize:]
    directory = []
    for index in range(numTables):
        table, data = structUnpack(directoryFormat, data)
        directory.append(table)
    return directory

def unpackTableData(data):
    directory = unpackDirectory(data)
    tables = {}
    for entry in directory:
        tag = entry["tag"]
        offset = entry["offset"]
        origLength = entry["origLength"]
        compLength = entry["compLength"]
        if offset > len(data) or offset < 0 or (offset + compLength) < 0:
            tableData = ""
        elif offset + compLength > len(data):
            tableData = data[offset:]
        else:
            tableData = data[offset:offset+compLength]
        if compLength < origLength:
            try:
                td = zlib.decompress(tableData)
                tableData = td
            except zlib.error:
                tableData = None
        tables[tag] = tableData
    return tables

def unpackMetadata(data, decompress=True, parse=True):
    header = unpackHeader(data)
    data = data[header["metaOffset"]:header["metaOffset"]+header["metaLength"]]
    if decompress and data:
        data = zlib.decompress(data)
    if parse and data:
        data = ElementTree.fromstring(data)
    return data

def unpackPrivateData(data):
    header = unpackHeader(data)
    data = data[header["privOffset"]:header["privOffset"]+header["privLength"]]
    return data

# -----------------------
# Support: Report Helpers
# -----------------------

def findUniqueFileName(path):
    if not os.path.exists(path):
        return path
    folder = os.path.dirname(path)
    fileName = os.path.basename(path)
    fileName, extension = os.path.splitext(fileName)
    stamp = time.strftime("%Y-%m-%d %H-%M-%S %Z")
    newFileName = "%s (%s)%s" % (fileName, stamp, extension)
    newPath = os.path.join(folder, newFileName)
    # intentionally break to prevent a file overwrite.
    # this could happen if the user has a directory full
    # of files with future time stamped file names.
    # not likely, but avoid it all the same.
    assert not os.path.exists(newPath)
    return newPath


# ---------------
# Public Function
# ---------------

def validateFont(path, options, writeFile=True):
    # start the reporter
    if options.outputFormat == "html":
        reporter = HTMLReporter()
    elif options.outputFormat == "text":
        reporter = TextReporter()
    else:
        raise NotImplementedError
    # log the title
    reporter.logTitle("Report: %s" % os.path.basename(path))
    # log fileinfo
    reporter.logFileInfo("FILE", os.path.basename(path))
    reporter.logFileInfo("DIRECTORY", os.path.dirname(path))
    # run tests and log results
    f = open(path, "rb")
    data = f.read()
    f.close()
    haveReadError = False
    canDisplayMetadata = True
    while 1:
        # the goal here is to locate as many errors as possible in
        # one session, rather than stopping validation at the first
        # instance of an error. to do this, each test function returns
        # two booleans indicating the following:
        #   1. errors were found that should cease all further tests.
        #   2. errors were found, but futurther tests can proceed.
        # this is important because displaying metadata for a file
        # with errors must not happen.

        # header
        reporter.logTestTitle("Header")
        stoppingError, nonStoppingError = testHeader(data, reporter)
        if nonStoppingError:
            canDisplayMetadata = False
        if stoppingError:
            haveReadError = True
            break
        # data blocks
        reporter.logTestTitle("Data Blocks")
        stoppingError, nonStoppingError = testDataBlocks(data, reporter)
        if nonStoppingError:
            canDisplayMetadata = False
        if stoppingError:
            haveReadError = True
            break
        # table directory
        reporter.logTestTitle("Table Directory")
        stoppingError, nonStoppingError = testTableDirectory(data, reporter)
        if nonStoppingError:
            canDisplayMetadata = False
        if stoppingError:
            haveReadError = True
            break
        # table data
        reporter.logTestTitle("Table Data")
        stoppingError, nonStoppingError = testTableData(data, reporter)
        if nonStoppingError:
            canDisplayMetadata = False
        if stoppingError:
            haveReadError = True
            break
        # metadata
        reporter.logTestTitle("Metadata")
        stoppingError, nonStoppingError = testMetadata(data, reporter)
        if nonStoppingError:
            canDisplayMetadata = False
        if stoppingError:
            haveReadError = True
            break
        # done
        break
    reporter.haveReadError = haveReadError
    # report the metadata
    if not haveReadError and canDisplayMetadata:
        metadata = getMetadataForDisplay(data)
        reporter.logMetadata(metadata)
    # get the report
    report = reporter.getReport()
    # write
    reportPath = None
    if writeFile:
        # make the output file name
        if options.outputFileName is not None:
            fileName = options.outputFileName
        else:
            fileName = os.path.splitext(os.path.basename(path))[0]
            fileName += "_validate"
            if options.outputFormat == "html":
                fileName += ".html"
            else:
                fileName += ".txt"
        # make the output directory
        if options.outputDirectory is not None:
            directory = options.outputDirectory
        else:
            directory = os.path.dirname(path)
        # write the file
        reportPath = os.path.join(directory, fileName)
        reportPath = findUniqueFileName(reportPath)
        f = open(reportPath, "wb")
        f.write(report)
        f.close()
    return reportPath, report

################################################################################
############################### Fontforge test #################################
################################################################################

# Generate a WOFF file with fontforge.
fontname=sys.argv[1]
woffname = "%s.woff" % os.path.splitext(os.path.basename(fontname))[0]
font=fontforge.open(fontname)
font.generate(woffname)
font.close()

# Use the W3C's validator (code above) on the output WOFF file.
class defaultOptions(object):
   def __init__( self ):
       self.outputFormat = "text"
reportPath, report = validateFont(woffname, defaultOptions(), False)
os.remove(woffname)

# Check the validation report and raise assertion if an ERROR is found.
for line in report.split(os.linesep):
    if line.startswith("ERROR"):
        raise Exception(line)
    print(line)
