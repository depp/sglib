import sys

def run(obj):
    import buildtool.build.nix as nix
    b = nix.build_nix(obj)
    if b.build():
        sys.exit(0)
    else:
        sys.exit(1)
