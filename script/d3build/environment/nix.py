# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .environment import BaseEnvironment
from . import schema
from ..shell import get_output, describe_proc
from ..error import ConfigError, format_block
from ..source import SRCTYPE_EXT, _base
from ..log import logfile
import io
import os
import tempfile

SCHEMA = schema.Schema()
(SCHEMA
    .prog ('CC', 'The C compiler')
    .prog ('CXX', 'The C++ compiler')
    .prog ('LD', 'The linker')
    .flags('CPPFLAGS', 'C and C++ preprocessor flags')
    .paths('CPPPATH', 'C and C++ header search paths')
    .defs ('DEFS', 'C and C++ preprocessor definitions')
    .flags('CFLAGS', 'C compilation flags')
    .flags('CXXFLAGS', 'C++ compilation flags')
    .flags('CWARN', 'C warning flags')
    .flags('CXXWARN', 'C++ warning flags')
    .flags('LDFLAGS', 'Linker flags')
    .flags('LIBS', 'Linker flags specifying libraries')
)

SCHEMA_OSX = schema.Schema()
(SCHEMA_OSX
    .paths('FPATH', 'Framework search paths')
    .unique_flags('FRAMEWORKS', 'Frameworks to link in')
)

class NixEnvironment(BaseEnvironment):
    """A Unix-like configuration environment."""
    __slots__ = [
        # Base compilation variables in this environment.
        'base_vars',
        # Additional variables specified by the user.
        'user_vars',
    ]

    def __init__(self, config):
        super(NixEnvironment, self).__init__(config)
        self.schema.update(SCHEMA)
        if config.platform == 'osx':
            self.schema.update(SCHEMA_OSX)

        if config.config == 'debug':
            cflags = ('-O0', '-g')
        else:
            cflags = ('-O2', '-g')
        varset = self.varset(
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

        self.base_vars = varset
        self.user_vars = self.varset_parse(config.variables, strict=False)

    @staticmethod
    def cc_command(varset, output, source, sourcetype, *,
                   depfile=None, external=False):
        """Get the command to compile the given source file."""
        varset = self.varset_merge([self.base_vars, varset, self.user_vars])
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

    @staticmethod
    def ld_command(varset, output, sources, sourcetypes):
        """Get the command to link the given source."""
        varset = self.varset_merge([self.base_vars, varset, self.user_vars])
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

    def dump(self, *, file):
        """Dump information about the environment."""
        super(NixEnvironment, self).dump(file=file)
        file = logfile(2)
        print('Base build variables:', file=file)
        self.base_vars.dump(file, indent='  ')
        print('User build variables:', file=file)
        self.user_vars.dump(file, indent='  ')

    def header_paths(self, *, base, paths):
        """Create build variables that include a header search path."""
        return self.varset(
            CPPPATH=tuple(_base(base, path) for path in paths))

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
        varset = self.varset_parse(flags)
        varset['CXXFLAGS'] = varset['CFLAGS']
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
        varset = self.varset_parse(flags)
        varset['CXXFLAGS'] = varset['CFLAGS']
        return varset

    def frameworks(self, flist):
        """Specify a list of frameworks to use."""
        if self._config.platform != 'osx':
            return super(NixEnvironment, self).frameworks(flist)
        flist = tuple(flist)
        if not all(isinstance(x, str) for x in flist):
            raise TypeError('expected a list of strings')
        return self.varset(FRAMEWORKS=flist)

    def test_compile_link(self, source, sourcetype, base_varset, varsets):
        """Try different build variable sets to find one that works."""
        log = io.StringIO()
        print('Testing compilation and linking.', file=log)
        print('Source type: {}'.format(sourcetype), file=log)
        log.write(format_block(source))
        if base_varset is not None:
            print('Base build variables:', file=log)
            base_varset.dump(log, indent='  ')
        with tempfile.TemporaryDirectory() as dirpath:
            file_c = os.path.join(dirpath, 'config' + SRCTYPE_EXT[sourcetype])
            file_obj = os.path.join(dirpath, 'config.o')
            file_out = os.path.join(dirpath, 'out')
            with open(file_c, 'w') as fp:
                fp.write(source)
            for varset in varsets:
                print('Test build variables:', file=log)
                varset.dump(log, indent='  ')
                test_varset = self.varset_merge([base_varset, varset])
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
