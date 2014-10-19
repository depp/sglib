# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .environment import BaseEnvironment
from .variable import BuildVariables
from ..shell import get_output, describe_proc
from ..error import ConfigError, format_block
from ..source import SRCTYPE_EXT
import io
import os
import tempfile

def cc_command(varset, output, source, sourcetype, *,
               depfile=None, external=False):
    """Get the command to compile the given source file."""
    if sourcetype in ('c', 'objc'):
        cc, cflags, cwarn = 'CC', 'CFLAGS', 'CWARN'
    elif sourcetype in ('c++', 'objc++'):
        cc, cflags, cwarn = 'CXX', 'CXXFLAGS', 'CXXWARN'
    else:
        raise ValueError('invalid source type')

    try:
        cc = varset[cc]
    except KeyError:
        raise ConfigError('{} is not set'.format(cc))
    cflags = varset.get(cflags, ())
    if external:
        cwarn = ()
    else:
        cwarn = varset.get(cwarn, ())

    cmd = [cc, '-o', output, source, '-c']
    if depfile is not None:
        cmd.extend(('-MF', depfile, '-MMD', '-MP'))
    cmd.extend('-I' + p for p in varset.get('CPPPATH', ()))
    cmd.extend('-F' + p for p in varset.get('FPATH', ()))
    cmd.extend(mkdef(k, v) for k, v in varset.get('DEFS', ()))
    cmd.extend(varset.get('CPPFLAGS', ()))
    cmd.extend(cwarn)
    cmd.extend(cflags)
    return cmd

def ld_command(varset, output, sources, sourcetypes):
    """Get the command to link the given source."""
    if 'c++' in sourcetypes or 'objc++' in sourcetypes:
        cc = 'CXX'
    else:
        cc = 'CC'
    try:
        cc = varset[cc]
    except KeyError:
        raise ConfigError('{} is not set'.format(cc))

    cmd = [cc]
    cmd.extend(varset.get('LDFLAGS', ()))
    cmd.extend(('-o', output))
    cmd.extend(sources)
    cmd.extend('-F' + p for p in varset.get('FPATH', ()))
    cmd.extend(varset.get('LIBS', ()))
    for framework in varset.get('FRAMEWORKS', ()):
        cmd.extend(('-framework', framework))
    return cmd

class NixEnvironment(BaseEnvironment):
    """A Unix-like configuration environment."""
    __slots__ = ['base_vars']

    def __init__(self, config):
        super(NixEnvironment, self).__init__(config)

        if config.config == 'debug':
            cflags = ('-O0', '-g')
        else:
            cflags = ('-O2', '-g')
        varset = BuildVariables(
            CC='cc',
            CXX='c++',
            CFLAGS=cflags,
            CXXFLAGS=cflags,
        )
        varset.update(
            CFLAGS=('-pthread',),
            CXXFLAGS=('-pthread',),
            LDFLAGS=('-pthread',))
        if config.warnings:
            varset.update(
                CWARN=tuple(
                    '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                    '-Wwrite-strings -Wmissing-prototypes '
                    '-Werror=implicit-function-declaration '
                    .split()),
                CXXWARN=tuple(
                    '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                    .split()))
        if config.werror:
            varset.update(
                CWARN=('-Werror',),
                CXXWARN=('-Werror',))
        if config.platform == 'linux':
            varset.update(
                LDFLAGS=('-Wl,--as-needed', '-Wl,--gc-sections'))
        if config.platform == 'osx':
            varset.update(
                LDFLAGS=('-Wl,-dead_strip', '-Wl,-dead_strip_dylibs'))

        varset.update_parse(config.variables, strict=False)
        self.base_vars = varset

    def log_info(self):
        """Log basic information about the environment."""
        super(NixEnvironment, self).log_info()

        out = self.logfile(2)
        print('Build variables:', file=out)
        self.base_vars.dump(out, indent='  ')
        print(file=out)

    def pkg_config(self, spec):
        """Run the pkg-config tool and return the build variables."""
        cmdname = 'pkg-config'
        flags = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = get_output(
                [cmdname, '--silence-errors', arg, spec])
            if retcode:
                stdout, retcode = get_output(
                    [cmdname, '--print-errors', '--exists', spec],
                    combine_output=True)
                raise ConfigError('{} failed'.format(cmdname), details=stdout)
            flags[varname] = stdout
        varset = BuildVariables.parse(flags)
        varset.CXXFLAGS = varset.CFLAGS
        return varset

    def sdl_config(self, version):
        """Run the sdl-config tool and return the build variables.

        The version should either be 1 or 2.
        """
        if version == 1:
            cmdname = 'sdl-config'
        elif version == 2:
            cmdname = 'sdl2-config'
        else:
            raise ValueError('unknown SDL version: {!r}'.format(version))
        flags = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = get_output([cmdname, arg])
            if retcode:
                raise ConfigError('{} failed'.format(cmdname), details=stderr)
            flags[varname] = stdout
        varset = BuildVariables.parse(flags)
        varset.CXXFLAGS = varset.CFLAGS
        return varset

    def frameworks(self, flist):
        """Specify a list of frameworks to use."""
        if self.platform != 'osx':
            return super(NixEnvironment, self).frameworks(flist)
        flist = tuple(flist)
        if not all(isinstance(x, str) for x in flist):
            raise TypeError('expected a list of strings')
        return BuildVariables(FRAMEWORKS=flist)

    def test_compile_link(self, source, sourcetype, base_varset, varsets):
        """Try different build variable sets to find one that works."""
        log = io.StringIO()
        print('Testing compilation and linking.', file=log)
        print('Source type: {}'.format(sourcetype), file=log)
        log.write(format_block(source))
        print('Base build variables:', file=log)
        base_varset.dump(log, indent='    ')
        with tempfile.TemporaryDirectory() as dirpath:
            file_c = os.path.join(dirpath, 'config' + SRCTYPE_EXT[sourcetype])
            file_obj = os.path.join(dirpath, 'config.o')
            file_out = os.path.join(dirpath, 'out')
            with open(file_c, 'w') as fp:
                fp.write(source)
            for varset in varsets:
                print('Test build variables:', file=log)
                varset.dump(log, indent='    ')
                test_varset = BuildVariables.merge([base_varset, varset])
                cmds = [
                    cc_command(test_varset, file_obj, file_c,
                               sourcetype, external=True),
                    ld_command(test_varset, file_out, [file_obj],
                               [sourcetype]),
                ]
                for cmd in cmds:
                    try:
                        stdout, retcode = get_output(
                            cmd, combine_output=True)
                    except ConfigError as ex:
                        print(ex, file=log)
                        break
                    else:
                        log.write(describe_proc(cmd, stdout, retcode))
                        if retcode:
                            break
                else:
                    print('Success', file=log)
                    return varset
        raise ConfigError(
            'could not compile and link test file',
            details=log.getvalue())
