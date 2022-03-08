#!/usr/bin/env python3

import pathlib
import subprocess
import sys


def update(potfile, pofile):
    header = []
    with open(pofile, encoding="utf-8") as fp:
        for l in fp:
            l = l.strip()
            if l.startswith("#"):
                header.append(l)
            else:
                break

    updated_trans = pathlib.Path("updates") / pathlib.Path(pofile).name

    print(f"Updating {pofile} with {potfile}...")

    cmd = ["msgmerge", "-N", "-s", pofile, potfile, "-o", pofile]
    if not updated_trans.exists():
        cmd.append("--for-msgfmt")
    subprocess.check_call(cmd)

    if updated_trans.exists():
        print(f"Updating {pofile} with {updated_trans}...")
        subprocess.check_call(
            ["msgmerge", "--for-msgfmt", "-s", str(updated_trans), pofile, "-o", pofile]
        )

    print(f"Reappending header to {pofile}...")
    with open(pofile, encoding="utf-8") as fp:
        data = fp.read()
    with open(pofile, "w", encoding="utf-8") as fp:
        print("\n".join(header), file=fp)
        fp.write(data)


def update_all(potfile):
    for f in pathlib.Path().glob("*.po"):
        update(potfile, str(f))


if __name__ == "__main__":
    update_all(sys.argv[1])
