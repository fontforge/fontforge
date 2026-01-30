import fontforge

# This test checks whether strings passed to fontforge for Python cause crashes
# due to % not being escaped and forwarded to printf with invalid format
# specifier or missing arguments.

# Formatting is done completely from python and these functions accept only a
# single string argument.

# shouldn't produce a warning
fontforge.logWarning("SANITIZATION %% CHECK")
# should produce a warning
fontforge.logWarning("SANITIZATION % CHECK")
# should produce a single warning
fontforge.logWarning("%SANIT%IZATION CHECK%")

# Titles aren't formatted at the moment, but they're tested here anyway as a
# sanity check in case future versions pass the dialog title to printf.

# shouldn't produce a warning
fontforge.postError("%%POSTERROR", "SANITIZATION %% CHECK")
# should produce a warning
fontforge.postError("POST % ERROR%", "SANITIZATION % CHECK")
# should produce a single warning
fontforge.postError("%POST ER%ROR%", "%SANIT%IZATION CHECK%")

# shouldn't produce a warning
fontforge.postNotice("POST %% NOTICE", "SANITIZATION %% CHECK")
# should produce a warning
fontforge.postNotice("POST % NOTICE", "SANITIZATION % CHECK")
# should produce a single warning
fontforge.postNotice("%POST %NOT%ICE%", "%SANIT%IZATION CHECK%")
