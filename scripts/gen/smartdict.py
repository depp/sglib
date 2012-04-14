import UserDict
from gen.path import Path
import gen.source as source

# This is NOT a KeyError and it is NOT supposed to be
class UnknownKey(Exception):
    def __init__(self, key):
        self.key = key
    def __repr__(self):
        return 'UnknownKey(%r)' % (self.key,)
    def __str__(self):
        return 'unknown key: %r' % (self.key,)

class UnsetKey(AttributeError):
    __slots__ = ['key', 'fallback']
    def __init__(self, key, fallback=()):
        self.key = key
        self.fallback = tuple(fallback)
    def __repr__(self):
        if self.fallback:
            return 'UnsetKey(%r)' % (self.key,)
        else:
            return 'UnsetKey(%r, %r)' % (self.key, self.fallback)
    def __str__(self):
        if self.fallback:
            return 'key not set: %s (defaults from %s)' % \
                (self.key, ', '.join(self.fallback))
        else:
            return 'key not set: %s' % self.key

class KeyValueError(ValueError):
    __slots__ = ['key', 'value', 'reason']
    def __init__(self, key, value, reason):
        self.key = key
        self.value = value
        self.reason = reason
    def __repr__(self):
        return ('KeyValueError(%r, %r, %r)' %
                (self.key, self.value, self.reason))
    def __str__(self):
        return ('invalid value for %s key: %r (%s)' %
                (self.key, self.value, self.reason))

class KeyTypeError(TypeError):
    __slots__ = ['key', 'value', 'reason']
    def __init__(self, key, value, reason):
        self.key = key
        self.value = value
        self.reason = reason
    def __repr__(self):
        return ('KeyTypeError(%r, %r, %r)' %
                (self.key, self.value, self.reason))
    def __str__(self):
        return ('invalid type for %s key: %r (%s)' %
                (self.key, self.value, self.reason))

class Key(object):
    """Abstract superclass for SmartDict keys."""

    def __init__(self, name):
        self.name = name

    def __get__(self, instance, owner):
        if instance is None:
            return self
        try:
            return instance._props[self.name]
        except KeyError:
            pass
        value = self.default(instance)
        try:
            return self.check(value)
        except ValueError, ex:
            raise KeyValueError(self.name, value, ex)
        except TypeError, ex:
            raise KeyTypeError(self.name, value, ex)

    def default(self, instance):
        """Return the default value if this variable is not set."""
        raise UnsetKey(self.name)

    def default_var(self, instance, name):
        """Use another variable as default.

        Used to implement default in subclasses.
        """
        try:
            return getattr(instance, name)
        except UnsetKey, ex:
            raise UnsetKey(self.name, (ex.prop,) + ex.fallback)

    def __set__(self, instance, value):
        try:
            nvalue = self.check(value)
        except ValueError, ex:
            raise KeyValueError(self.name, value, ex)
        except TypeError, ex:
            raise KeyTypeError(self.name, value, ex)
        instance._props[self.name] = nvalue

    def __delete__(self, instance):
        del instance._props[self.name]

    def check(self, value):
        """Check whether a definition is valid and normalize it.

        Returns a normalized version of the value or raises an
        exception.  The default implementation simply returns the
        input.
        """
        return value

    def combine(self, value, other):
        """Combine two definitions into one definition.

        By default, the new definition replaces the old one.
        """
        return other

    def as_string(self, value):
        return value

class UnicodeKey(Key):
    """Unicode smart dictionary key."""
    __slots__ = ['name']

    @staticmethod
    def check(self, value):
        if isinstance(value, unicode):
            value
        elif isinstance(value, str):
            return unicode(value, 'ascii')
        raise TypeError('not a string: %s' % (value,))

    @staticmethod
    def as_string(value):
        return value.encode('utf-8')

_TRUE = frozenset(['1', 'true', 'yes', 'on'])
_FALSE = frozenset(['0', 'false', 'no', 'off'])
class BoolKey(Key):
    """Boolean smart dictionary key.

    Any of the values 1, True, Yes, or On is interpreted as true.  0,
    False, No, and Off are false.  Not case sensitive.
    """
    __slots__ = ['name', '_default']

    def __init__(self, name, default=None):
        Key.__init__(self, name)
        self._default = default

    @staticmethod
    def check(value):
        if isinstance(value, str):
            v = value.lower()
            if v in _TRUE:
                return True
            elif v in _FALSE:
                return False
            else:
                raise ValueError('invalid boolean: %r' % (value,))
        elif isinstance(value, bool):
            return value
        raise TypeError('invalid boolean: %r' % (value,))

    def default(self, instance):
        if self._default is not None:
            return self._default
        raise UnsetKey(self.name)

    @staticmethod
    def as_string(value):
        return '1' if value else '0'

class EnumKey(Key):
    """An enumeration variable key.

    Possible values are strings from a fixed list.
    """
    __slots__ = ['name', '_default', '_vals']

    def __init__(self, name, vals, default=None):
        Key.__init__(self, name)
        self._default = default
        self._vals = frozenset(vals)

    def check(self, value):
        if not isinstance(value, str):
            raise TypeError('value must be string')
        if value not in self._vals:
            raise ValueError('value must be one of %s' %
                             (', '.join(self._vals),))
        return value

    def default(self, instance):
        if self._default is not None:
            return self._default
        raise UnsetKey(self.name)

class AtomKey(Key):
    """An atom key."""

    @staticmethod
    def check(value):
        if not isinstance(value, str):
            raise TypeError('invalid atom type: %r' % (value,))
        if not source.is_atom(value):
            raise ValueError('invalid atom: %r' % (value,))
        return value

class AtomListKey(Key):
    """An atom list dictionary key.

    When combining two values, duplicates will be removed.
    """

    @staticmethod
    def check(value):
        if isinstance(value, str):
            value = value.split()
        else:
            value = list(value)
        nvalue = []
        for x in value:
            if x in nvalue:
                continue
            if not (isinstance(x, str) and source.is_atom(x)):
                raise ValueError('invalid atom: %r' % (x,))
            nvalue.append(x)
        return value

    @staticmethod
    def combine(value, other):
        if not value:
            return other
        if not other:
            return value
        nvalue = list(value)
        for p in other:
            if p not in nvalue:
                nvalue.append(p)
        return tuple(nvalue)

class PathKey(Key):
    """A path smart dictionary key."""

    @staticmethod
    def check(value):
        if isinstance(value, Path):
            return value
        else:
            return Path(value)

class PathListKey(Key):
    """A path list smart dictionary key.

    When combining values, duplicates will be removed.
    """

    @staticmethod
    def check(value):
        if isinstance(value, str):
            value = value.split()
        elif isinstance(value, Path):
            value = [value]
        else:
            value = list(value)
        nvalue = []
        for x in value:
            if isinstance(x, Path):
                pass
            elif isinstance(x, str):
                if not x:
                    continue
                x = Path(x)
            else:
                raise TypeError('invalid path: %r' % (x,))
            if x not in nvalue:
                nvalue.append(x)
        return tuple(nvalue)

    def default(self, instance):
        return ()

    @staticmethod
    def combine(value, other):
        if not value:
            return other
        if not other:
            return value
        nvalue = list(value)
        for p in other:
            if p not in nvalue:
                nvalue.append(p)
        return tuple(nvalue)

    def as_string(self, value):
        return ' '.join(p.posix for p in value)

class SmartDict(object, UserDict.DictMixin):
    """Abstract superclass for smart dictionary objects.

    This is an object between a dictionary and a structure.  It has a
    limited set of keys, and all keys can be accessed as attributes.
    When keys are set, the value will be checked and an exception
    raised for invalid values.  Two smart dictionaries can be
    combined, which will combine the keys for the underlying values in
    different ways.  Keys can also have default values, which may
    depend on the values of other keys.

    For example, if a key represents the command line flags for a
    program, then combining two values will result in the values being
    concatenated.  If a key represents a simple boolean flag, then one
    of the values will be discarded.

    Note: Accessing a key through its property will give the default
    value if it exists.  Accessing a key by index (e.g., obj['key'])
    will raise KeyError instead of giving a default value.
    """
    __slots__ = ['_props']

    def __init__(self, *args, **kw):
        """Create a new dictionary object.

        The arguments are a list of dictionary objects and a set of
        keyword arguments.  The resulting dictionary combines all of
        the keys in the listed object and all of the keys in the
        keyword arguments.

        Keys from dictionaries later in the list will take precedence
        from keys in dictionaries earlier in the list.  Keys from
        keyword arguments take precedence over all other keys.

        When combining dictionaries, all dictionaries must have the
        same class.
        """
        self._props = {}
        klass = self.__class__
        for key, val in kw.iteritems():
            try:
                kobj = getattr(klass, key)
            except AttributeError:
                raise UnknownKey(key)
            if not isinstance(kobj, Key):
                raise UnknownKey(key)
            kobj.__set__(self, val)
        if not args:
            return
        for arg in args:
            if not isinstance(arg, klass):
                raise TypeError('dictionary type mismatch')
        props = dict(args[0]._props)
        for obj in args[1:] + (self,):
            for key, val in obj._props.iteritems():
                try:
                    oldval = props[key]
                except KeyError:
                    props[key] = val
                else:
                    kobj = getattr(klass, key)
                    props[key] = kobj.combine(oldval, val)
        self._props = props

    def __getitem__(self, key):
        return self._props[key]

    def __setitem__(self, key, value):
        try:
            kobj = getattr(self.__class__, key)
        except AttributeError:
            raise UnknownKey(key)
        if not isinstance(kobj, Key):
            raise UnknownKey(key)
        kobj.__set__(self, value)

    def __delitem__(self, key):
        try:
            kobj = getattr(self.__class__, key)
        except AttributeError:
            raise UnknownKey(key)
        if not isinstance(self, Key):
            raise UnknownKey(key)
        del self._props[key]

    def __iter__(self):
        return iter(self._props)

    def keys(self):
        return self._props.keys()

    def iterkeys(self):
        return self._props.iterkeys()

    def iteritems(self):
        return self._props.iteritems()

    def __contains__(self, key):
        return key in self._props

    def dump(self):
        klass = self.__class__
        for key, val in sorted(self._props.iteritems(), key=lambda x: x[0]):
            kobj = getattr(klass, key)
            print key, '=', kobj.as_string(val)

    def set(self, **kw):
        """Convenience method for setting many keys."""
        
