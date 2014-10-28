# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..environment import BaseEnvironment
from ..schema import Schema
from ..shell import get_output, describe_proc
from ..error import ConfigError, format_block
from ..source import SRCTYPE_EXT, _base
from ..log import logfile
from ..util import yesno
import io
import os
import tempfile

SCHEMA = (Schema()
    .string('CC', 'The C compiler')
    .string('CXX', 'The C++ compiler')
    .list  ('CPPFLAGS', 'C and C++ preprocessor flags')
    .list  ('CFLAGS', 'C compilation flags')
    .list  ('CXXFLAGS', 'C++ compilation flags')
    .list  ('CWARN', 'C warning flags')
    .list  ('CXXWARN', 'C++ warning flags')
    .list  ('LDFLAGS', 'Linker flags')
    .list  ('LIBS', 'Linker flags specifying libraries')
)

class NixEnvironment(BaseEnvironment):
    """A Unix-like configuration environment."""
    __slots__ = [
        # Configuration variables.
        '_configuration', '_warnings', '_werror',
        # Base compilation variables in this environment.
        'base_vars',
    ]

    def __init__(self, config):
        super(NixEnvironment, self).__init__(config)
        self.schema.update_schema(SCHEMA)

        self._configuration = self.get_variable('CONFIG', 'release')
        self._warnings = self.get_variable_bool('WARNINGS', True)
        self._werror = self.get_variable_bool(
            'WERROR', self._configuration=='debug')
        if self._configuration == 'debug':
            cflags = ['-O0', '-g']
        elif self._configuration == 'release':
            cflags = ['-O2', '-g']
        else:
            cflags = []
        base_vars = self.schema.varset(
            CC='cc',
            CXX='c++',
            CFLAGS=cflags,
            CXXFLAGS=cflags,
        )
        self.schema.update(
            base_vars,
            CFLAGS=['-pthread'],
            CXXFLAGS=['-pthread'],
            LDFLAGS=['-pthread'])
        if self._warnings:
            self.schema.update(
                base_vars,
                CWARN=
                    '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                    '-Wwrite-strings -Wmissing-prototypes '
                    '-Werror=implicit-function-declaration '
                    .split(),
                CXXWARN=
                    '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                    .split())
        if self._werror:
            self.schema.update(
                base_vars,
                CWARN=['-Werror'],
                CXXWARN=['-Werror'])
        if config.platform == 'linux':
            self.schema.update(
                base_vars,
                LDFLAGS=['-Wl,--as-needed', '-Wl,--gc-sections'])
        if config.platform == 'osx':
            self.schema.update(
                base_vars,
                LDFLAGS=['-Wl,-dead_strip', '-Wl,-dead_strip_dylibs'])

        user_vars = {}
        for varname, value in self.variable_list:
            try:
                vardef = self.schema[varname]
            except KeyError:
                continue
            self.variable_unused.discard(varname)
            user_vars[varname] = vardef.parse(value)

        self.base_vars = self.schema.merge([base_vars, user_vars])

    @staticmethod
    def cc_command(varset, output, source, sourcetype, *,
                   depfile=None, external=False):
        """Get the command to compile the given source file."""
        varset = self.schema.merge([self.base_vars, varset])
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
        cmd.extend(varset.get('CPPFLAGS', ()))
        cmd.extend(cwarn)
        cmd.extend(cflags)
        return cmd

    @staticmethod
    def ld_command(varset, output, sources, sourcetypes):
        """Get the command to link the given source."""
        varset = self.schema.merge([self.base_vars, varset])
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
        cmd.extend(varset.get('LIBS', ()))
        return cmd

    def dump(self, *, file):
        """Dump information about the environment."""
        super(NixEnvironment, self).dump(file=file)
        file = logfile(2)
        print('Configuration:', file=file)
        print('  CONFIG={}'.format(self._configuration), file=file)
        print('Enable extra warnings:', file=file)
        print('  WARNINGS={}'.format(yesno(self._warnings)), file=file)
        print('Treat warnings as errors:', file=file)
        print('  WERROR={}'.format(yesno(self._werror)), file=file)
        print('Compilation flags:', file=file)
        self.schema.dump(self.base_vars, file=file, indent='  ')

    def define(self, definition):
        """Create build variables that define a preprocessor variable."""
        return {'CPPFLAGS:': ['-D' + definition]}

    def header_path(self, path, *, system=False):
        """Create build variables that include a header search path."""
        flag = '-I' if system else '-iquote'
        return {'CPPFLAGS': [flag + path]}

    def framework_path(self, path):
        """Create build variables that include a framework search path."""
        return {'LIBS': ['-F' + path]}

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
        varset = self.schema.parse(flags)
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
        varset = self.schema.parse(flags)
        varset['CXXFLAGS'] = varset['CFLAGS']
        return varset

    def frameworks(self, flist):
        """Specify a list of frameworks to use."""
        if self._config.platform != 'osx':
            return super(NixEnvironment, self).frameworks(flist)
        if not isinstance(flist, list):
            raise TypeError('frameworks must be list')
        libs = []
        for framework in flist:
            if not isinstance(framework, str):
                raise TypeError('expected a list of strings')
            libs.extend(('-framework', framework))
        return self.schema.varset(LIBS=libs)

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
                test_varset = self.schema.merge([base_varset, varset])
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
