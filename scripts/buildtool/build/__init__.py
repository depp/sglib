import sys
import platform

def run(obj):
    s = platform.system()
    if s == 'Linux':
        import buildtool.build.nix as nix
        build = nix.build_nix(obj)
    elif s == 'Darwin':
        import buildtool.build.nix as nix
        build = nix.build_macosx(obj)
    elif s == 'Windows':
        import buildtool.build.msvc as msvc
        build = msvc.build(obj)
    else:
        print >>sys.stderr, 'error: unspported system: %s' % s
        sys.exit(1)
    if build.build(obj):
        sys.exit(0)
    else:
        sys.exit(1)
