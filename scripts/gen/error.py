import platform

class ConfigError(Exception):
    """Project configuration failed."""

    __slots__ = ['reason', 'details']

    def __init__(self, reason, details=None):
        self.reason = reason
        self.details = details

class BuildError(Exception):
    def advice(self):
        funcs = []
        sys = platform.system()
        if sys == 'Linux':
            distname, version, id = platform.linux_distribution()
            if distname:
                funcs.append('linux_' + distname.lower())
            funcs.append('linux')
        elif sys == 'Darwin':
            release, verinfo, machine = platform.mac_ver()
            funcs.append('osx')
        for func in funcs:
            try:
                fn = getattr(self, 'advice_' + func)
            except AttributeError:
                continue
            r = fn()
            if r is not None:
                return r

_PKG_NAME = {
    'alsa': 'ALSA development package',
    'gtkglext': 'GtkGLExt',
}

_DEB_PKG = {
    'alsa': 'libasound2-dev',
    'gtkglext': 'libgtkglext1-dev',
}

class MissingPackage(BuildError):
    def __init__(self, name):
        self.name = name
    def __str__(self):
        return 'could not find "%s" package' % self.name
    def advice_linux_debian(self):
        human_name = _PKG_NAME.get(self.name, self.name)
        try:
            pkgident = _DEB_PKG[self.name]
        except KeyError:
            return None
        return 'To install %s, run\n    $ sudo apt-get install %s\n' % \
            (human_name, pkgident)
