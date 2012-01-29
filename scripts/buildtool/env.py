import os

class EnvError(Exception):
    pass

def find_program(*progs):
    """Find the absolute path to a program if it exists.

    Return None if the program does not exist.  Try all names listed
    in order.
    """
    for prog in progs:
        for path in os.environ['PATH'].split(os.path.pathsep):
            if not path:
                continue
            path = os.path.join(path, prog)
            if os.access(path, os.X_OK):
                return path

PROGS = ('CC', 'CXX')
FLAGS = ('CPPFLAGS', 'CFLAGS', 'CXXFLAGS', 'CWARN', 'CXXWARN',
         'LDFLAGS', 'LIBS', 'ARCHS', 'ARCH')

def _accum(x, y):
    for prog in PROGS:
        try:
            x[prog] = y[prog]
        except KeyError:
            pass
    for flags in FLAGS:
        try:
            v2 = y[flags]
        except KeyError:
            continue
        try:
            v1 = x[flags]
        except KeyError:
            x[flags] = v2
        else:
            x[flags] = v1 + v2

class Environment(object):
    def __init__(self, *args, **kw):
        env = {}
        for e in args:
            if not isinstance(e, Environment):
                raise TypeError
            _accum(env, e._env)
        for k, v in kw.iteritems():
            if k in PROGS:
                if not v:
                    continue
                if isinstance(v, str):
                    v = find_program(v)
                else:
                    v = find_program(*v)
            elif k in FLAGS:
                if isinstance(v, str):
                    v = v.split()
                elif isinstance(v, list) or isinstance(v, tuple):
                    v = list(v)
                else:
                    raise EnvError('%s must be a string or list' % k)
            else:
                raise EnvError('unknown variable: %s' % repr(k))
            env[k] = v
        self._env = env

    def __getattr__(self, k):
        try:
            return self._env[k]
        except KeyError:
            if k in FLAGS:
                return []
            raise AttributeError(k)

    def __add__(self, other):
        if not isinstance(other, Environment):
            print repr(other)
            raise TypeError
        e = {}
        _accum(e, self._env)
        _accum(e, other._env)
        r = Environment()
        r._env = e
        return r
