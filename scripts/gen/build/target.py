from __future__ import with_statement
import os
import subprocess
import shutil
import re
import sys
from gen.env import Environment, VarRef
from gen import path

Path = path.Path

class Target(object):
    """Abstract base class for targets."""
    def __hash__(self):
        for x in self.output():
            return hash(x)
        return hash(())

    def __eq__(self, other):
        return self is other

    def __ne__(self, other):
        return self is not other

    def __repr__(self):
        n = self.__class__.__name__
        i = n.rfind('.')
        if i >= 0:
            n = n[i + 1:]
        x = '<none>'
        for i in self.output():
            x = i
            if isinstance(x, Path):
                x = x.posix
            break
        return '<%s %s>' % (n, x)

    def late(self):
        """Return whether the target should be built late.

        This is a bit of a hack.  The build process generally has
        three phases: early, make, and late.  Anything not marked as
        'late' is divided into 'early' or 'make' based on whether it
        can be built using a makefile.  Since anything in the early
        phase will force all of its prerequesites to be early as well,
        marking a target late allows its prerequisites to be built
        during the make phase.
        """
        return False

    def distinput(self):
        """Iterate over input that should be distributed."""
        return self.input()

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

def mkescapearg(x):
    try:
        return SPECIAL_ARG.sub(escape, x)
    except ValueError:
        raise ValueError('invalid character in %r' % (x,))

def mkmkarg(x):
    if isinstance(x, str):
        x = mkescapearg(x)
    elif isinstance(x, Path):
        x = mkescapearg(x.posix)
    elif isinstance(x, VarRef):
        x = str(x)
    elif isinstance(x, tuple):
        a = ''
        for i in x:
            if isinstance(i, str):
                i = mkescapearg(i)
            elif isinstance(i, Path):
                i = mkescapearg(i.posix)
            elif isinstance(i, VarRef):
                i = str(i)
            else:
                raise TypeError('invalid command argument: %r' % (x,))
            a += i
        x = a
    else:
        raise TypeError('invalid command argument: %r' % (x,))
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

class Commands(Target):
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

    def build(self, verbose):
        env = self._env
        if not verbose:
            try:
                name = self.name
            except AttributeError:
                verbose = True
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
        environ = env.environ
        if not environ:
            environ = None
        for cmd in self.commands():
            if not cmd:
                raise ValueError('command is empty')
            args = [mkarg(arg) for arg in cmd]
            if verbose:
                print ' '.join(args)
            if (cmd[0] == 'cp' and len(cmd) == 3 and
                isinstance(cmd[1], Path) and isinstance(cmd[2], Path)):
                shutil.copyfile(args[1], args[2])
            else:
                proc = subprocess.Popen(args, env=environ)
                status = proc.wait()
                if status:
                    return False
        return True

    def write_rule(self, f, generic, verbose):
        """Write the makefile rule for this target to the given file."""
        cmds = [[mkmkarg(arg) for arg in cmd]
                for cmd in self.commands()]
        out1 = None
        for x in self.output():
            if out1 is None:
                out1 = mkmkdep(x)
            else:
                f.write('%s: %s\n' % (mkmkdep(x), out1))
        itarg = ' '.join(mkmkdep(x) for x in self.input())
        if generic and isinstance(self, CC):
            itarg += ' $(dirs_missing)'
        if not out1:
            raise ValueError('target has no outputs')
        if itarg:
            f.write('%s: %s\n' % (out1, itarg))
        else:
            f.write('%s:\n' % (out1,))
        if cmds:
            try:
                name = self.name
            except AttributeError:
                name = None
            else:
                name = name()
                if generic:
                    cmds = [['$(Q%s)' % name]] + \
                        [['$(QS)' + cmd[0]] + cmd[1:] for cmd in cmds]
                elif not verbose:
                    cmds = [['@echo %s $@' % name]] + \
                        [['@' + cmd[0]] + cmd[1:] for cmd in cmds]
            for cmd in cmds:
                f.write('\t' + ' '.join(cmd) + '\n')

class DepTarget(Target):
    """Target which does nothing, but depends on other targets.

    The name should be a string.
    """
    __slots__ = ['_name', '_deps']
    def __init__(self, name, deps):
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

    def build(self, verbose):
        return True

    def write_rule(self, f, generic, verbose):
        """Write the makefile rule for this target to the given file."""
        otarg = ' '.join(mkmkdep(x) for x in self.output())
        itarg = ' '.join(mkmkdep(x) for x in self.input())
        if itarg:
            f.write('%s: %s\n' % (otarg, itarg))
        else:
            f.write('%s:\n' % (otarg,))


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

class StaticFile(Target):
    """A target which creates a file from a string.

    Requires an _env and _dest slot, both of which are initialized by
    the constructor.  Subclasess should override the 'write' method,
    which takes a file-like object and writes data to the file.
    """
    def __init__(self, dest):
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        self._dest = dest

    def input(self):
        return iter(())

    def output(self):
        yield self._dest

    def build(self, verbose):
        print 'FILE', self._dest.posix
        with open(self._dest.native, 'wb') as f:
            self.write(f)
        return True

class Template(Target):
    """Target which creates an file from a template.

    Takes a lookup function as an argument.
    """
    __slots__ = ['_dest', '_src', '_regex', '_lookup']

    def __init__(self, dest, src, lookup, regex=r'\${(\w+)}'):
        if not isinstance(dest, Path):
            raise TypeError('destination must be path')
        if not isinstance(src, Path):
            raise TypeError('source must be path')
        self._dest = dest
        self._src = src
        self._regex = regex
        self._lookup = lookup

    def input(self):
        yield self._src

    def output(self):
        yield self._dest

    def build(self, verbose):
        print 'TEMPLATE', self._dest.posix
        data = open(self._src.native, 'rb').read()
        def repl(m):
            w = m.group(1)
            v = self._lookup(w)
            if v is None:
                print >>sys.stderr, '%s: unknown variable %r' % (w, w)
                return m.group(0)
            return v
        data = re.sub(self._regex, repl, data)
        with open(self._dest.native, 'wb') as f:
            f.write(data)
        return True

    def distinput(self):
        return iter(())
