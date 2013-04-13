import platform
import re
from build.error import ConfigError

def darwin_version():
    if platform.system() != 'Darwin':
        return None
    m = re.match(r'(\d+)(?:\.(\d+))(?:\.(\d+))', platform.release())
    if not m:
        return None
    return tuple(int(m.group(n) or '0') for n in range(1, 4))

def osx_version():
    dversion = darwin_version()
    if dversion is None:
        return None
    x, y, z = dversion
    if (x, y) <= (1, 3): return 10, 0
    if x <= 5: return 10, 1
    if x <= 6: return 10, 2
    if x <= 7: return 10, 3
    if x <= 8: return 10, 4
    if x <= 9: return 10, 5
    if x <= 10: return 10, 6
    if x <= 11: return 10, 7
    return 10, 8

def default_release_archs(version=False):
    """Get default release architectures for this platform."""
    if version is False:
        version = osx_version()
    if version and version <= (10, 5):
        return ('ppc', 'i386', 'x86_64')
    return ('i386', 'x86_64')

MACHINE_ARCH = {
    'Power Macintosh': 'ppc',
    'i386': 'i386',
    'x86_64': 'x86_64',
}

def default_debug_archs():
    """Get default debug architectures for this platform."""
    if platform.system() != 'Darwin':
        return ('i386',)
    m = platform.machine()
    try:
        arch = MACHINE_ARCH[m]
    except KeyError:
        raise ConfigError(
            'could not determine architecture for machine: {!r}'
            .format(m))
    return (arch,)
