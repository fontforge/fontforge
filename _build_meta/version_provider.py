# FontForge version provider for scikit-build-core
# Extracts version from CMakeLists.txt and converts YYYYMMDD to CalVer YYYY.M.D

from __future__ import annotations

import re
from pathlib import Path
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from collections.abc import Mapping

__all__ = ["dynamic_metadata"]


def dynamic_metadata(
    field: str,
    settings: Mapping[str, Any],
) -> str:
    if field != "version":
        msg = f"This provider only supports 'version', not {field!r}"
        raise RuntimeError(msg)

    # Read CMakeLists.txt from project root
    cmake_file = Path("CMakeLists.txt")
    if not cmake_file.exists():
        msg = "CMakeLists.txt not found"
        raise RuntimeError(msg)

    content = cmake_file.read_text(encoding="utf-8")

    # Extract version from: project(fontforge VERSION 20251009 ...)
    match = re.search(r"project\s*\(\s*fontforge\s+VERSION\s+(\d{8})", content)
    if not match:
        msg = "Could not find project VERSION in CMakeLists.txt"
        raise RuntimeError(msg)

    version_str = match.group(1)

    # Parse YYYYMMDD format
    year = version_str[0:4]
    month = int(version_str[4:6])  # int() strips leading zero
    day = int(version_str[6:8])

    # Return CalVer format: YYYY.M.D
    return f"{year}.{month}.{day}"
