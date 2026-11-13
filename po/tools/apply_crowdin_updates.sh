#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$script_dir/../.." && pwd)"

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <crowdin-zip> [potfile]"
  echo "example: $0 '~/Downloads/FontForge (translations).zip'"
  echo "example: $0 '~/Downloads/FontForge (translations).zip' $repo_root/build/po/FontForge.pot"
  exit 1
fi

zip_path="$1"
updates_dir="$repo_root/po/updates"
default_pot="$repo_root/build/po/FontForge.pot"
pot_path="${2:-$default_pot}"

if [[ ! -f "$zip_path" ]]; then
  echo "error: Crowdin ZIP not found: $zip_path"
  exit 1
fi

if [[ ! -f "$pot_path" ]]; then
  echo "error: POT file not found: $pot_path"
  echo "please run this manually first: ninja -C '$repo_root/build' potfiles"
  exit 1
fi

if ! command -v unzip >/dev/null 2>&1; then
  echo "error: unzip command is required but not found"
  exit 1
fi

rm -rf "$updates_dir"
mkdir -p "$updates_dir"

tmpdir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmpdir"
  rm -rf "$updates_dir"
}
trap cleanup EXIT

unzip -q -o "$zip_path" -d "$tmpdir"

count=0
while IFS= read -r po_file; do
  base_name="$(basename "$po_file")"
  cp "$po_file" "$updates_dir/$base_name"
  count=$((count + 1))
done < <(find "$tmpdir" -type f -path '*/po/*.po' | sort)

if [[ $count -eq 0 ]]; then
  echo "error: no po/*.po files found inside ZIP: $zip_path"
  exit 1
fi

echo "Extracted $count PO files into $updates_dir"
echo "Running updater with POT: $pot_path"
python3 "$repo_root/po/tools/update.py" "$pot_path"

echo "Done. Updated PO files in $repo_root/po"
echo "Cleaned temporary updates in $updates_dir"
