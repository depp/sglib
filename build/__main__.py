import build.data
import build.xml
from build.path import Path
import sys

ENV = {
    'os': 'linux',
    'flag': {
        'audio': 'coreaudio',
        'jpeg': 'coregraphics',
        'png': 'coregraphics',
        'type': 'coretext',
        'video-recording': True,
    }
}

try:
    buildfile = build.data.BuildFile()
    build.xml.parse_file(buildfile, Path(sys.argv[1]), ENV)
except ValueError as ex:
    print(ex, file=sys.stderr)
    sys.exit(1)
