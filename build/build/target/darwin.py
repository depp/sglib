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

def default_release_archs():
    """Get default release architectures for this platform."""
    version = darwin_version()
    if version and version[0] < 10:
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
