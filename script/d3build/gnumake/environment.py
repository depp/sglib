# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..environment import BaseEnvironment
from ..schema import SchemaBuilder
from ..shell import get_output, describe_proc
from ..error import ConfigError, format_block
from ..source import SRCTYPE_EXT, _base
from ..log import logfile
from ..util import yesno
import os
import tempfile

SCHEMA = (SchemaBuilder()
    .string('CC', 'The C compiler')
    .string('CXX', 'The C++ compiler')
    .list  ('CPPFLAGS', 'C and C++ preprocessor flags')
    .list  ('CFLAGS', 'C compilation flags')
    .list  ('CXXFLAGS', 'C++ compilation flags')
    .list  ('CWARN', 'C warning flags')
    .list  ('CXXWARN', 'C++ warning flags')
    .list  ('LDFLAGS', 'Linker flags')
    .list  ('LIBS', 'Linker flags specifying libraries')
    .bool  ('WERROR', 'Treat warnings as errors')
).value()

CONFIGS = ('Debug', 'Release')

class GnuMakeEnvironment(BaseEnvironment):
    """A build environment for the Gnu Make target."""
    __slots__ = ['base']

    def __init__(self, config):
        super(GnuMakeEnvironment, self).__init__(config)

        self.base = {}
        self.schema.update(
            self.base,
            CC='cc',
            CXX='c++',
            configs=CONFIGS)

        cflags = ['-O0', '-g']
        self.schema.update(
            self.base,
            CFLAGS=cflags,
            CXXFLAGS=cflags,
            configs=['Debug'])

        cflags = ['-O2', '-g']
        self.schema.update(
            self.base,
            CFLAGS=cflags,
            CXXFLAGS=cflags,
            configs=['Release'])

        # user_vars = {}
        # for varname, value in self.variable_list:
        #     try:
        #         vardef = self.schema[varname]
        #     except KeyError:
        #         continue
        #     self.variable_unused.discard(varname)
        #     user_vars[varname] = vardef.parse(value)

        # self.base_vars = self.schema.merge([base_vars, user_vars])

    @property
    def schema(self):
        return SCHEMA

    @property
    def configs(self):
        return CONFIGS

    def cc_command(self, config, varset, output, source, sourcetype, *,
                   depfile=None, external=False):
        """Get the command to compile the given source file."""
        if sourcetype in ('c', 'objc'):
            cc, cflags, cwarn = 'CC', 'CFLAGS', 'CWARN'
        elif sourcetype in ('c++', 'objc++'):
            cc, cflags, cwarn = 'CXX', 'CXXFLAGS', 'CXXWARN'
        else:
            raise ValueError('invalid source type')

        try:
            cc = varset[config, cc]
        except KeyError:
            raise ConfigError('{} is not set'.format(cc))
        cflags = varset.get((config, cflags), ())
        if external:
            cwarn = ()
        else:
            cwarn = varset.get((config, cwarn), ())

        cmd = [cc, '-o', output, source, '-c']
        if depfile is not None:
            cmd.extend(('-MF', depfile, '-MMD', '-MP'))
        cmd.extend(varset.get((config, 'CPPFLAGS'), ()))
        cmd.extend(cwarn)
        cmd.extend(cflags)
        return cmd

    def ld_command(self, config, varset, output, sources, sourcetypes):
        """Get the command to link the given source."""
        if 'c++' in sourcetypes or 'objc++' in sourcetypes:
            cc = 'CXX'
        else:
            cc = 'CC'
        try:
            cc = varset[config, cc]
        except KeyError:
            raise ConfigError('{}.{} is not set'.format(config, cc))

        cmd = [cc]
        cmd.extend(varset.get((config, 'LDFLAGS'), ()))
        cmd.extend(('-o', output))
        cmd.extend(sources)
        cmd.extend(varset.get((config, 'LIBS'), ()))
        return cmd

    def dump(self, *, file):
        """Dump information about the environment."""
        super(GnuMakeEnvironment, self).dump(file=file)
        file = logfile(2)
        print('Compilation flags:', file=file)
        self.schema.dump(self.base, file=file, indent='  ')

    def define(self, definition):
        """Create build variables that define a preprocessor variable."""
        return {'CPPFLAGS:': ['-D' + definition]}

    def header_path(self, path, *, configs=None, system=False):
        """Create build variables that include a header search path."""
        flag = '-I' if system else '-iquote'
        return self.varset(CPPFLAGS=[flag + path], configs=configs)

    def library(self, path, *, configs=None):
        """Create build variables that link with a library."""
        return self.varset(LIBS=[path], configs=configs)

    def library_path(self, path):
        """Create build variables that include a library search path."""
        return self.varset(LIBS=['-L' + path], configs=configs)

    def framework(self, name):
        """Create build variables that link with a framework."""
        if self._config.platform != 'osx':
            return super(GnuMakeEnvironment, self).framework(name)
        return self.varset(LIBS=['-framework', name], configs=configs)

    def framework_path(self, path):
        """Create build variables that include a framework search path."""
        if self._config.platform != 'osx':
            return super(GnuMakeEnvironment, self).framework_path(path)
        return self.varset(LIBS=['-F', path], configs=configs)

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
        flags['CXXFLAGS'] = flags['CFLAGS']
        varset = self.schema.parse(flags, strict=True, configs=self.configs)
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
        flags['CXXFLAGS'] = flags['CFLAGS']
        varset = self.schema.parse(flags, strict=True, configs=self.configs)
        return varset

    def test_compile(self, source, sourcetype, base_varset, varsets,
                     *, configs=None, use_all=False, external=True,
                     link=True):
        """Try different build variable sets to find one that works."""
        if configs is None:
            configs = self.configs
        log = logfile(2)
        print('Testing compilation and linking.', file=log)
        print('Source type: {}'.format(sourcetype), file=log)
        log.write(format_block(source))
        if base_varset is not None:
            print('Base build variables:', file=log)
            self.schema.dump(base_varset, file=log, indent='  ')
        result = {}
        with tempfile.TemporaryDirectory() as dirpath:
            file_c = os.path.join(dirpath, 'config' + SRCTYPE_EXT[sourcetype])
            file_obj = os.path.join(dirpath, 'config.o')
            file_out = os.path.join(dirpath, 'out')
            with open(file_c, 'w') as fp:
                fp.write(source)
            for varset in varsets:
                print('Test build variables:', file=log)
                self.schema.dump(varset, file=log, indent='  ')
                test_varset = self.schema.merge(
                    [self.base, base_varset, result, varset])
                for config in configs:
                    cmds = [self.cc_command(
                        config, test_varset, file_obj, file_c,
                        sourcetype, external=external)]
                    if link:
                        cmds.append(self.ld_command(
                            config, test_varset, file_out, [file_obj],
                            [sourcetype]))
                    success = False
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
                        success = True
                    if not success:
                        break
                else:
                    print('Success', file=log)
                    if use_all:
                        self.schema.update_varset(result, varset)
                    else:
                        return varset
        if use_all:
            return result
        raise ConfigError('could not compile and link test file')
