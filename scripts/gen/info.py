from gen.path import Path
import gen.smartdict as smartdict
import re
import gen.env as env

_IS_TITLE = re.compile(r'^[-\w]+(?: [-\w]+)*$')
def is_title(x):
    return bool(_IS_TITLE.match(x))

_IS_FILENAME = re.compile(r'^[_a-zA-Z0-9][-_a-zA-Z0-9]*$')
def is_filename(x):
    return bool(_IS_FILENAME.match(x))

_IS_DOMAIN = re.compile(r'^[a-z0-9](?:[-a-z0-9]*[a-z0-9])?' 
                        r'(?:\.[a-z0-9](?:[-a-z0-9]*[a-z0-9])?)*$')
def is_domain(x):
    return bool(_IS_DOMAIN.match(x))

_IS_VERSION = re.compile(r'^[-+.A-Za-z0-9]+$')
def is_version(x):
    return bool(_IS_VERSION.match(x))

_IS_HASH = re.compile(r'^[0-9a-f]+$')
def is_hash(x):
    return bool(_IS_HASH.match(x))

_IS_CVAR = re.compile(r'^[-_A-Za-z0-9]+(?:\.[-_A-Za-z0-9]+)+')
def is_cvar(x):
    return bool(_IS_CVAR.match(x))

class Title(smartdict.Key):
    """A title environment variable.

    Must be a sequence of 'words' separated by individual spaces.
    Words may contain alphanumeric characterss, hyphens, and
    underscores.
    """

    def __init__(self, name, default=None):
        smartdict.Key.__init__(self, name)
        self._default = default

    def default(self, instance):
        if self._default is not None:
            return self.default_var(instance, self._default)
        return smartdict.Key.default(self, instance)

    @staticmethod
    def check(value):
        if not isinstance(value, str):
            raise TypeError('invalid title')
        if not is_title(value):
            raise ValueError('invalid title')
        return value

class Filename(smartdict.Key):
    """A filename environment variable.

    Must be a sequence of alphanumeric characters, underscores, and
    hyphens.  A hyphen may not be the first character.  The default
    value is derived from another key by replacing invalid character
    sequences with an underscore.
    """

    def __init__(self, name, default=None):
        smartdict.Key.__init__(self, name)
        self._default = default

    def default(self, instance):
        if self._default is not None:
            value = self.default_var(instance, self._default)
            return re.sub('[^-A-Za-z0-9]+', '_', value)
        return smartdict.Key.default(self, instance)

    @staticmethod
    def check(value):
        if not isinstance(value, str):
            raise TypeError('invalid filename')
        if not is_filename(value):
            raise ValueError('invalid filename')
        return value

class DomainName(smartdict.Key):
    """A domain name environment variable."""

    def check(self, value):
        if not isinstance(value, str):
            raise TypeError('invalid domain name')
        if not is_domain(value):
            raise ValueError('invalid domain name')
        return value

class Version(smartdict.Key):
    """A version number environment variable."""

    def check(self, value):
        if not isinstance(value, str):
            raise TypeError('invalid version')
        if not is_version(value):
            raise ValueError('invalid version')
        return value

class Hash(smartdict.Key):
    """A hash (e.g., git SHA1) environment variable"""

    def check(self, value):
        if not isinstance(value, str):
            raise TypeError('invalid hash')
        if not is_hash(value):
            raise ValueError('invalid hash')
        return value

class CVars(smartdict.Key):
    """A set of configuration variables for SGLib.

    When combined, later definitions of the same cvar override
    earlier definitions.
    """

    @staticmethod
    def check(value):
        value = list(value)
        value.reverse()
        nvalue = []
        keys = set()
        for v in value:
            if not isinstance(v, tuple):
                raise TypeError('must be a tuple: %r' % (v,))
            if not v:
                raise ValueError('empty key/value')
            k = v[0]
            v = v[1:]
            if not is_cvar(k):
                raise ValueError('not a cvar name: %r' % (k,))
            v = env.Flag.check(v)
            if k not in keys:
                keys.add(k)
                nvalue.append((k, v))
        nvalue.reverse()
        return tuple(nvalue)

    def default(self, instance):
        return ()

    @staticmethod
    def as_string(value):
        return '; '.join('%s=%s' % (k, env.Flag.as_string(v))
                         for (k, v) in value)

    @staticmethod
    def combine(value, other):
        if not value:
            return other
        if not other:
            return value
        nvalue = list(other)
        nvalue.reverse()
        keys = set(k for (k, v) in nvalue)
        for v in reversed(value):
            if v[0] not in keys:
                nvalue.append(v)
        nvalue.reverse()
        return tuple(nvalue)

class ProjectInfo(smartdict.SmartDict):
    """Project info dictionary."""
    __slots__ = ['_props']

    PKG_NAME       = Title('PKG_NAME')
    PKG_IDENT      = DomainName('PKG_IDENT')
    PKG_FILENAME   = Filename('PKG_FILENAME', 'PKG_NAME')
    PKG_URL        = smartdict.Key('PKG_URL')
    PKG_EMAIL      = smartdict.Key('PKG_EMAIL')
    PKG_COPYRIGHT  = smartdict.UnicodeKey('PKG_COPYRIGHT')

    PKG_SG_VERSION   = Version('PKG_SG_VERSION')
    PKG_SG_COMMIT    = Hash('PKG_SG_COMMIT')
    PKG_APP_VERSION  = Version('PKG_APP_VERSION')
    PKG_APP_COMMIT   = Hash('PKG_APP_COMMIT')

    # Default config vars for running all executables
    DEFAULT_CVARS = CVars('DEFAULT_CVARS')

class ExecInfo(smartdict.SmartDict):
    """Executable info dictionary."""
    __slots__ = ['_props']

    EXE_NAME     = Title('EXE_NAME')
    EXE_MAC      = Title('EXE_MAC', 'EXE_NAME')
    EXE_LINUX    = Filename('EXE_LINUX', 'EXE_NAME')
    EXE_WINDOWS  = Title('EXE_WINDOWS', 'EXE_NAME')
    EXE_MACICON  = Filename('EXE_MACICON')

    EXE_APPLE_CATEGORY = DomainName('PKG_APPLE_CATEGORY')

    # Default config vars for running the executable
    DEFAULT_CVARS = CVars('DEFAULT_CVARS')
