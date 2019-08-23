#!/bin/bash
# Generates FontForge.spec to be used with rpmbuild and the dist archive

set -e

BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION=$(grep -m1 project.*VERSION "$BASE/../../CMakeLists.txt"  | sed 's/.*VERSION \([0-9]*\).*/\1/g')

cat "$BASE/FontForge.spec.in" | sed "s/@PROJECT_VERSION@/$VERSION/g" > FontForge.spec
