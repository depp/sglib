import os
import re

class EnvError(Exception):
    pass

class UnknownProperty(EnvError):
    def __init__(self, name):
        self.name = name
    def __repr__(self):
        return 'UnknownProperty(%r)' % (self.name,)
    def __str__(self):
        return 'unknown environment variable: %r' % (self.name,)

class UnsetProperty(EnvError):
    def __init__(self, prop, fallback=()):
        self.prop = prop
        self.fallback = fallback
    def __repr__(self):
        if self.fallback:
            return 'UnsetProperty(%r)' % self.prop
        else:
            return 'UnsetProperty(%r, %r)' % (self.prop, self.fallback)
    def __str__(self):
        if self.fallback:
            return 'property not set: %s (defaults from %s)' % \
                (self.prop, ', '.join(self.fallback))
        else:
            return 'property not set: %s' % self.prop

class InvalidProperty(EnvError):
    def __init__(self, prop, value):
        self.prop = prop
        self.value = value
    def __repr__(self):
        return 'InvalidProperty(%r, %r)' % (self.prop, self.value)
    def __str__(self):
        return 'invalid value for %s property: %r' % \
            (self.prop, self.value)

IS_ENVVAR = re.compile(r'^[_A-Z]+$')

class EnvVar(object):
    """An environment variable."""

    def __init__(self, name):
        self.name = name

    def get(self, instance):
        try:
            return instance._props[self.name]
        except KeyError:
            pass
        value = self.default(instance)
        isvalid, nvalue = self.check(value)
        if not isvalid:
            raise InvalidProperty(self.name, value)
        return nvalue

    def default(self, instance):
        """Return the default value if this variable is not set."""
        raise UnsetProperty(self.name)

    def default_var(self, instance, name):
        """Use another variable as default.

        Used to implement default in subclasses.
        """
        try:
            return getattr(instance, name)
        except UnsetProperty, ex:
            raise UnsetProperty(self.name, (ex.prop,) + ex.fallback)

    def set(self, instance, value):
        isvalid, nvalue = self.check(value)
        if isvalid:
            instance._props[self.name] = nvalue
        else:
            raise InvalidProperty(self.name, value)

    def delete(self, instance):
        del instance._props[self.name]

    def check(self, value):
        """Check whether a definition is valid.

        Returns (isvalid, newvalue), where isvalid is a boolean
        indicating whether the value is valid, and newvalue is a
        modified version of the value to store.
        """
        return True, value

    def combine(self, value, other):
        """Combine two definitions into one definition.

        By default, the new definition replaces the old one.
        """
        return other

    def as_string(self, value):
        return value

class Program(EnvVar):
    """Program path environment variable.

    This can be set to a list or a string.  If set to a list, the PATH
    is searched for each string until one is found.
    """

    def get(self, instance):
        try:
            return instance._paths[self.name]
        except KeyError:
            pass
        value = EnvVar.get(self, instance)
        abspath = instance.prog_path(*value)
        instance._paths[self.name] = abspath
        return abspath

    def check(self, value):
        if isinstance(value, str):
            return True, [value]
        else:
            return True, list(value)

class Flags(EnvVar):
    """Program flags environment variable.

    These can be set from strings or iterables, but are always stored
    internally as a list of strings.
    """
    def __init__(self, name, default=None):
        EnvVar.__init__(self, name)
        self._default = default

    def check(self, value):
        if isinstance(value, str):
            return True, value.split()
        nvalue = list(value)
        for f in nvalue:
            if not isinstance(f, str):
                return False, None
        return True, nvalue

    def default(self, instance):
        if self._default is not None:
            return self.default_var(instance, self._default)
        return []

    def combine(self, value, other):
        return value + other

    def as_string(self, value):
        return ' '.join(value)

IS_TITLE = re.compile(r'^[-\w]+(?: [-\w]+)*$')
IS_FILE = re.compile(r'\w+')
IS_DOMAIN = re.compile(r'^[a-z0-9](?:[-a-z0-9]*[a-z0-9])?' 
                       r'(?:\.[a-z0-9](?:[-a-z0-9]*[a-z0-9])?)*$')

class Title(EnvVar):
    """A title environment variable.

    Must be a sequence of 'words' separated by individual spaces.
    Words may contain alphanumeric characterss, hyphens, and
    underscores.
    """

    def __init__(self, name, default=None):
        EnvVar.__init__(self, name)
        self._default = default

    def default(self, instance):
        if self._default is not None:
            return self.default_var(instance, self._default)
        EnvVar.default(self, instance)

    def check(self, value):
        if IS_TITLE.match(value):
            return True, value
        return False, None

class Filename(EnvVar):
    """A filename environment variable.

    Must be a sequence of alphanumeric characters and underscores.  No
    hyphens are allowed.
    """

    def __init__(self, name, default=None):
        EnvVar.__init__(self, name)
        self._default = default

    def default(self, instance):
        if self._default is not None:
            value = self.default_var(instance, self._default)
            return re.sub('[-_ ]+', '_', value)
        EnvVar.default(self, instance)

    def check(self, value):
        if IS_FILE.match(value):
            return True, value
        return False, None

class DomainName(EnvVar):
    """A domain name environment variable."""

    def check(self, value):
        if IS_DOMAIN.match(value):
            return True, value
        return False, None

PROGS = [Program(p) for p in ('CC', 'CXX', 'LD')]
FLAGS = [Flags(p) for p in
         ('CPPFLAGS', 'CFLAGS', 'CXXFLAGS', 'CWARN', 'CXXWARN',
          'LDFLAGS', 'LIBS', 'ARCHS')]
PKG = [
    Title('PKG_NAME'),
    DomainName('PKG_IDENT'),
    Filename('PKG_FILENAME', 'PKG_NAME'),
    EnvVar('PKG_URL'),
    EnvVar('PKG_EMAIL'),
    Title('EXE_NAME', 'PKG_NAME'),
    Title('EXE_MAC', 'EXE_NAME'),
    Filename('EXE_LINUX', 'EXE_NAME'),
    Title('EXE_WINDOWS', 'EXE_NAME'),
]
VARS = PROGS + FLAGS + PKG
VARS = dict((v.name, v) for v in VARS)

class Environment(object):
    def __init__(self, *args, **kw):
        self._paths = {}
        self.environ = {}
        self._props = {}
        for k, v in kw.iteritems():
            try:
                var = VARS[k]
            except KeyError:
                raise UnknownProperty(k)
            var.set(self, v)
        if not args:
            return
        for arg in args:
            if not isinstance(arg, Environment):
                raise TypeError
            self.environ.update(arg.environ)
        props = dict(args[0]._props)
        for env in args[1:] + (self,):
            for k, v in env._props.iteritems():
                try:
                    x = props[k]
                except KeyError:
                    y = v
                else:
                    y = VARS[k].combine(x, v)
                props[k] = y
        self._props = props

    def __getattr__(self, name):
        if not IS_ENVVAR.match(name):
            return object.__getattr__(self, name)
        try:
            return self[name]
        except KeyError:
            raise AttributeError(name)

    def __setattr__(self, name, value):
        if not IS_ENVVAR.match(name):
            return object.__setattr__(self, name, value)
        try:
            self[name] = value
        except KeyError:
            raise AttributeError(name)

    def __delattr__(self, name):
        if not IS_ENVVAR.match(name):
            return object.__delattr__(self, name)
        try:
            del self[name]
        except KeyError:
            raise AttributeError(name)

    def __getitem__(self, name):
        var = VARS[name]
        return var.get(self)

    def __setitem__(self, name, value):
        var = VARS[name]
        return var.set(self, value)

    def __delitem__(self, name):
        var = VARS[name]
        return var.delete(self)

    def override(self, env):
        """Override variables with values from the other environment."""
        if not isinstance(env, Environment):
            raise TypeError
        p = self._props
        for k, v in env._props.iteritems():
            p[k] = v

    def set(self, **kw):
        """Set many variables at the same time."""
        for k, v in kw.iteritems():
            setattr(self, k, v)

    def dump(self):
        p = self._props
        for var in sorted(VARS.itervalues(), key=lambda x: x.name):
            try:
                v = p[var.name]
            except KeyError:
                continue
            print var.name, '=', var.as_string(v)

    def remove(self, key, flag):
        """Remove all flags which match 'flag' in var."""
        var = VARS[key]
        try:
            val = self._props[key]
        except KeyError:
            return
        if flag not in val:
            return
        nval = [f for f in val if f != flag]
        var.set(self, nval)

    def getenv(self, key):
        try:
            return self.environ[key]
        except KeyError:
            return os.environ[key]
 
    def prog_path(self, *progs):
        """Find the absolute path to a program if it exists."""
        paths = self.getenv('PATH').split(os.path.pathsep)
        for prog in progs:
            for path in paths:
                if not path:
                    continue
                progpath = os.path.join(path, prog)
                if os.access(progpath, os.X_OK):
                    return progpath
        raise Exception('program not found: %s' % prog)
