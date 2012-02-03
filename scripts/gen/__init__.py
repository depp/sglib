import sys
if sys.version_info[0] != 2 or not (5 <= sys.version_info[1] <= 7):
    sys.stderr.write('error: unsupported Python version (2.5-2.7 required)\n')
    sys.exit(1)
