from __future__ import with_statement
import os
import subprocess
import shutil
import re
import sys
from gen.env import Environment
from gen import path

Path = path.Path

def mkarg(x):
    if isinstance(x, str):
        pass
    elif isinstance(x, Path):
        x = x.native
    elif isinstance(x, tuple):
        a = ''
        for i in x:
            if isinstance(i, str):
                pass
            elif isinstance(i, Path):
                i = i.native
            else:
                raise TypeError('invalid command argument: %r' % (x,))
            a += i
        x = a
    else:
        raise TypeError('invalid command argument: %r' % (x,))
    return x

SPECIAL_ARG = re.compile('[^-_.+/A-Za-z0-9,=:]')
SPECIAL_DEP = re.compile('[^-_.+/A-Za-z0-9]')
def escape(x):
    c = x.group(0)
    if c == ' ':
        return '\\ '
    raise ValueError('invalid character: %r' % (c,))

def mkmkarg(x):
    if isinstance(x, str):
        pass
    elif isinstance(x, Path):
        x = x.posix
    elif isinstance(x, tuple):
        a = ''
        for i in x:
            if isinstance(i, str):
                pass
            elif isinstance(i, Path):
                i = i.posix
            else:
                raise TypeError('invalid command argument: %r' % (x,))
            a += i
        x = a
    else:
        raise TypeError('invalid command argument: %r' % (x,))
    try:
        x = SPECIAL_ARG.sub(escape, x)
    except ValueError:
        raise ValueError('invalid character in %r' % (x,))
    return x

def mkmkdep(x):
    if isinstance(x, str):
        pass
    elif isinstance(x, Path):
        x = x.posix
    else:
        raise TypeError('invalid make dependency: %r' % (x,))
    try:
        x = SPECIAL_DEP.sub(escape, x)
    except ValueError:
        raise ValueError('invalid character in %r' % (x,))
    return x

class Commands(object):
    """A target which runs a command.

    Requires an _env slot, which is initialized by the constructor.
    Subclasses should override the 'commands' method, which should
    return an iterable of commands to run.  Each command is itself an
    iterable of arguments, and each argument is either a string or a
    Path, or a tuple of strings and Paths to be concatenated.

    Subclasses may override the 'name' function, which should return
    the name which is printed when VERBOSE is not enabled.

    Subclasses should also override the input and output methods.
    """
    def __init__(self, env):
        if not isinstance(env, Environment):
            raise TypeError('env must be an Environment object')
        self._env = env

    def build(self):
        env = self._env
        quiet = not env.VERBOSE
        if quiet:
            try:
                name = self.name
            except AttributeError:
                quiet = False
            else:
                name = name()
                output = ''
                for o in self.output():
                    output = o.posix
                    break
                if output:
                    print name, output
                else:
                    print name
        for cmd in self.commands():
            if not cmd:
                raise ValueError('command is empty')
            args = [mkarg(arg) for arg in cmd]
            if not quiet:
                print ' '.join(args)
            if (cmd[0] == 'cp' and len(cmd) == 3 and
                isinstance(cmd[1], Path) and isinstance(cmd[2], Path)):
                shutil.copyfile(args[1], args[2])
            else:
                proc = subprocess.Popen(args)
                status = proc.wait()
                if status:
                    return False
        return True

    def write_rule(self, f, generic):
        """Write the makefile rule for this target to the given file."""
        cmds = [[mkmkarg(arg) for arg in cmd]
                for cmd in self.commands()]
        otarg = ' '.join(mkmkdep(x) for x in self.output())
        itarg = ' '.join(mkmkdep(x) for x in self.input())
        if not otarg:
            raise ValueError('target has no outputs')
        if itarg:
            f.write('%s: %s\n' % (otarg, itarg))
        else:
            f.write('%s:\n' % (otarg,))
        if cmds:
            try:
                name = self.name
            except AttributeError:
                name = None
            else:
                name = name()
                if generic:
                    cmds = [['$(QE %s)' % name]] + \
                        [['$(QS)' + cmd[0]] + cmd[1:] for cmd in cmds]
                elif not self._env.VERBOSE:
                    cmds = [['@echo %s $@' % name]] + \
                        [['@' + cmd[0]] + cmd[1:] for cmd in cmds]
            for cmd in cmds:
                f.write('\t' + ' '.join(cmd) + '\n')

class DepTarget(Commands):
    """Target which does nothing, but depends on other targets.

    The name should be a string.
    """
    __slots__ = ['_name', '_deps', '_env']
    def __init__(self, name, deps, env):
        Commands.__init__(self, env)
        if not isinstance(name, str):
            raise TypeError('name should be string')
        deps = list(deps)
        for d in deps:
            if not isinstance(d, (Path, str)):
                raise TypeError(
                    'deps should be sequence of paths and strings')
        self._name = name
        self._deps = deps

    def input(self):
        return iter(self._deps)

    def output(self):
        yield self._name

    def commands(self):
        return ()

class CC(Commands):
    """Compile a C, C++, or Objective C file.

    This is an abstract superclass for platform-specific versions.
    """
    __slots__ = ['_dest', '_src', '_env', '_sourcetype']
    def __init__(self, dest, src, env, sourcetype):
        Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be path')
        if not isinstance(src, Path):
            raise TypeError('src must be path')
        if sourcetype not in ('c', 'm', 'cxx', 'mm'):
            raise ValueError('not a C file type: %r (%s)' %
                             (sourcetype, dest.posix))
        self._dest = dest
        self._src = src
        self._sourcetype = sourcetype

    def input(self):
        yield self._src

    def output(self):
        yield self._dest

    def name(self):
        if self._sourcetype in ('c', 'm'):
            return 'CC'
        elif self._sourcetype in ('cxx', 'mm'):
            return 'CXX'

class LD(Commands):
    """Link an executable.

    This is an abstract superclass for platform-specific versions.
    """
    __slots__ = ['_dest', '_src', '_env']
    def __init__(self, dest, src, env):
        Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        src = list(src)
        for s in src:
            if not isinstance(s, Path):
                raise TypeError('src must be iterable of Path objects')
        self._dest = dest
        self._src = src

    def input(self):
        return iter(self._src)

    def output(self):
        yield self._dest

    def name(self):
        return 'LD'

class CopyFile(Commands):
    """A target which copies a file."""
    __slots__ = ['_dest', '_src', '_env']
    def __init__(self, dest, src, env):
        Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        if not isinstance(src, Path):
            raise TypeError('src must be Path')
        self._dest = dest
        self._src = src

    def input(self):
        yield self._src

    def output(self):
        yield self._dest

    def commands(self):
        return [['cp', self._src, self._dest]]

    def name(self):
        return 'COPY'

class StaticFile(object):
    """A target which creates a file from a string.

    Requires an _env and _dest slot, both of which are initialized by
    the constructor.  Subclasess should override the 'write' method,
    which takes a file-like object and writes data to the file.
    """
    def __init__(self, dest, env):
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        if not isinstance(env, Environment):
            raise TypeError('env must be Environment')
        self._dest = dest
        self._env = env

    def input(self):
        return iter(())

    def output(self):
        yield self._dest

    def build(self):
        env = self._env
        print 'FILE', self._dest.posix
        with open(self._dest.native, 'wb') as f:
            self.write(f)
        return True

class Template(object):
    """Target which creates an file from a template."""
    __slots__ = ['_dest', '_src', '_env']
    def __init__(self, dest, src, env):
        if not isinstance(dest, Path):
            raise TypeError('destination must be path')
        if not isinstance(src, Path):
            raise TypeError('source must be path')
        self._dest = dest
        self._src = src
        self._env = env

    def input(self):
        yield self._src

    def output(self):
        yield self._dest

    def build(self):
        print 'TEMPLATE', self._dest.posix
        data = open(self._src.native, 'rb').read()
        env = self._env
        def repl(m):
            w = m.group(1)
            try:
                return env[w]
            except KeyError:
                pass
            print >>sys.stderr, '%s: unknown variable %r' % (w, w)
            return m.group(0)
        data = re.sub(r'\${(\w+)}', repl, data)
        with open(self._dest.native, 'wb') as f:
            f.write(data)
        return True
