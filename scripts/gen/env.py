import os
import re
import platform
from gen.path import Path
import gen.smartdict as smartdict

_IS_VARREF = re.compile(r'^[_a-zA-Z0-9]+$')
def is_varref(x):
    return _IS_VARREF.match(x)

class VarRef(object):
    """A reference to a Makefile variable.

    This is used to automatically generate Makefile rules.  A VarRef
    can be used in place of a program or command line flag.
    """
    __slots__ = ['_name']

    def __init__(self, name):
        if not isinstance(name, str):
            raise TypeError('variable name must be str')
        if not is_varref(name):
            raise ValueError('variable name must match [_a-zA-Z0-9]+')
        self._name = name

    def __repr__(self):
        return 'VarRef(%r)' % (self._name,)

    def __str__(self):
        return '$(%s)' % (self._name,)

_PROG_CACHE = {}
class Program(smartdict.Key):
    """Program path environment variable.

    This can be set to an iterable of strings, a single string, or a
    VarRef.  If not set to a VarRef, the PATH is searched until an
    executable is found, and the absolute path is returned.
    """
    __slots__ = ['name']

    def notfound(self, instance):
        name = self.name
        value = ' '.join(smartdict.Key.__get__(self, instance, None))
        path = instance.getenv('PATH')
        raise Exception('program not found: %s; search: %s; PATH: %s' %
                        (name, value, path))

    def __get__(self, instance, owner):
        if instance is None:
            return self

        # Get the value from the environment's cache
        try:
            abspath = instance._paths[self.name]
        except KeyError:
            pass
        else:
            if abspath is None:
                self.notfound(instance)
            return abspath

        # Fall back to the global cache
        prognames = smartdict.Key.__get__(self, instance, owner)
        if isinstance(prognames, VarRef):
            return prognames

        # Get the cache for this value of PATH / PATHEXT
        path = instance.getenv('PATH')
        paths = path.split(os.path.pathsep)
        if platform.system() == 'Windows':
            exts = instance.getenv('PATHEXT')
            pkey = (path, exts)
            exts = exts.split(os.path.pathsep)
            extset = frozenset(ext.lower() for ext in exts)
            def with_exts(name):
                ext = os.path.splitext(name)[1]
                if ext and ext.lower() in extset:
                    return [name]
                else:
                    return [name + ext for ext in exts]
        else:
            pkey = path
            def with_exts(name):
                return [name]
        try:
            pcache = _PROG_CACHE[pkey]
        except KeyError:
            pcache = {}
            _PROG_CACHE[pkey] = pcache

        # Get the cached absolute path
        for progname in prognames:
            try:
                abspath = pcache[progname]
            except KeyError:
                abspath = None
                fnames = with_exts(progname)
                for path in paths:
                    if not path:
                        continue
                    for fname in fnames:
                        progpath = os.path.join(path, fname)
                        if os.access(progpath, os.X_OK):
                            abspath = progpath
                            break
                    else:
                        continue
                    break
                pcache[progname] = abspath
            if abspath is not None:
                break

        # Cache the result in the local cache
        instance._paths[self.name] = abspath

        if abspath is None:
            self.notfound(instance)
        return abspath

    @staticmethod
    def check(value):
        if isinstance(value, str):
            return (value,)
        elif isinstance(value, VarRef):
            return value
        else:
            value = tuple(value)
            for x in value:
                if not isinstance(x, str):
                    raise TypeError
        return value

    @staticmethod
    def as_string(value):
        return ' '.join(value)

# POSIX: special are |&;<>()$\\\"' *?[#~=%
#        non-special are !+,-./:@]^_`{}
# We escape some ones that are unnecessary
_SHELL_SPECIAL = re.compile(r'[^-A-Za-z0-9+,./:@^_]')
_SHELL_QSPECIAL = re.compile('["\\\\]')
def escape1(x):
    x = x.group(0)
    i = ord(x)
    if 32 <= i <= 126:
        return '\\' + x
    return '\\x%02x' % i
def shellescape(x):
    if _SHELL_SPECIAL.search(x):
        return '"' + _SHELL_QSPECIAL.sub(escape1, x) + '"'
    if not x:
        return "''"
    return x

def flagstr(x):
    if isinstance(x, str):
        return shellescape(x)
    elif isinstance(x, Path):
        return shellescape(x.posix)
    elif isinstance(x, VarRef):
        return str(x)
    else:
        a = ''
        for i in x:
            if isinstance(i, str):
                a += i
            elif isinstance(i, Path):
                a += i.posix
            else:
                raise TypeError
        return shellescape(a)

class Flag(smartdict.Key):
    """Program flag environment variable.

    This is not generally used by itself, but the methods are used
    to construct other types of keys.
    """
    __slots__ = ['name']

    @staticmethod
    def check(value):
        if isinstance(value, str):
            return value
        elif isinstance(value, (VarRef, Path)):
            return value
        else:
            value = tuple(value)
        for flag in value:
            if isinstance(flag, (str, Path, VarRef)):
                continue
            else:
                raise TypeError
        return value

    @staticmethod
    def as_string(value):
        return flagstr(value)

class Flags(smartdict.Key):
    """Program flags environment variable.

    These can be set from strings or iterables, but are always stored
    internally as a tuple.
    """
    __slots__ = ['name', '_default']

    def __init__(self, name, default=None):
        smartdict.Key.__init__(self, name)
        self._default = default

    @staticmethod
    def check(value):
        if isinstance(value, str):
            value = tuple(value.split())
        elif isinstance(value, (VarRef, Path)):
            return (value,)
        else:
            value = tuple(value)
        for flag in value:
            if isinstance(flag, (str, Path, VarRef)):
                continue
            elif isinstance(f, tuple):
                for x in f:
                    if not isinstance(x, (str, Path, VarRef)):
                        raise TypeError
        return value

    def default(self, instance):
        if self._default is not None:
            return self.default_var(instance, self._default)
        return ()

    @staticmethod
    def combine(value, other):
        return value + other

    @staticmethod
    def as_string(value):
        return ' '.join(flagstr(x) for x in value)

class BuildSettings(smartdict.SmartDict):
    """Build settings smart dictionary.

    These can only be set by the user.
    """
    __slots__ = ['_props']

    VERBOSE = smartdict.BoolKey('VERBOSE', False)
    CONFIG = smartdict.EnumKey(
        'CONFIG', ['debug', 'release'], default='release')

class Environment(smartdict.SmartDict):
    """Dictionary of build environment variables.

    This also includes a dictionary, environ, which contains OS
    environment variables.  The values in environ will be used for
    executing subprocess and for finding programs.
    """
    __slots__ = ['_paths', 'environ', '_props']

    CC  = Program('CC')
    CXX = Program('CXX')
    LD  = Program('LD')
    GIT = Program('GIT')

    CPPFLAGS  = Flags('CPPFLAGS')
    CPPPATH   = smartdict.PathListKey('CPPPATH')
    CFLAGS    = Flags('CFLAGS')
    CXXFLAGS  = Flags('CXXFLAGS')
    CWARN     = Flags('CWARN')
    CXXWARN   = Flags('CXXWARN')
    LDFLAGS   = Flags('LDFLAGS')
    LIBS      = Flags('LIBS')
    ARCHS     = Flags('ARCHS')

    def __init__(self, *args, **kw):
        smartdict.SmartDict.__init__(self, *args, **kw)
        self.environ = {}
        for env in args:
            self.environ.update(env.environ)
        self._paths = {}

    def remove_flag(self, key, flag):
        """Remove all flags which match 'flag' in var."""
        try:
            var = Environment.__dict__[key]
        except KeyError:
            raise smartdict.UnknownKey(name)
        if not isinstance(var, smartdict.Key):
            raise smartdict.UnknownKey(name)
        if not isinstance(var, Flags):
            raise Exception('not a flags variable: %s' % (key,))
        try:
            val = self._props[key]
        except KeyError:
            return
        if flag not in val:
            return
        nval = [f for f in val if f != flag]
        var.__set__(self, nval)

    def getenv(self, key):
        """Get the given OS environment variable."""
        try:
            return self.environ[key]
        except KeyError:
            return os.environ[key]
