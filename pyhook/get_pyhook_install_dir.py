import sys
import sysconfig as sc

install_dir_full = sys.argv[1]
is_apple = len(sys.argv) > 2 and sys.argv[2] == "APPLE"

if is_apple:
    p = sc.get_path("platlib", "posix_prefix", vars={"platbase": "."})
else:
    p = sc.get_path("platlib", vars={"platbase": "."})

    # sc.get_default_scheme() does not exist in Python < 3.10
    scheme = getattr(sc, "get_default_scheme", lambda: None)()
    # "posix_local" prepends "local/", causing double "local/" if installing to
    # /usr/local. Remove it unless installing directly to platbase (e.g., /usr).
    if (
        scheme == "posix_local" and
        install_dir_full != sc.get_config_var("platbase") and
        p.startswith("local/")
    ):
        p = p[6:]

print(p)
