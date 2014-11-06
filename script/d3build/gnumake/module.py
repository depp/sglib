# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..module import Module
from ..shell import get_output, describe_proc, escape_cmd
from ..error import ConfigError, format_block
from ..source import SRCTYPE_EXT
from ..log import logfile
import os
import tempfile

def cc_command(variables, variant, output, source, sourcetype, *,
               depfile=None, external=False):
    """Get the command to compile the given source file."""
    if sourcetype in ('c', 'objc'):
        cc, cflags, cwarn = 'CC', 'CFLAGS', 'CWARN'
    elif sourcetype in ('c++', 'objc++'):
        cc, cflags, cwarn = 'CXX', 'CXXFLAGS', 'CXXWARN'
    else:
        raise ValueError('invalid source type')

    cmd = [variables.get(variant, cc), '-o', output, source, '-c']
    if depfile is not None:
        cmd.extend(('-MF', depfile, '-MMD', '-MP'))
    cmd.extend(variables.get(variant, 'CPPFLAGS', ()))
    cmd.extend(variables.get(variant, cflags, ()))
    if not external:
        cmd.extend(variables.get(variant, cwarn, ()))
    return cmd

def ld_command(variables, variant, output, sources, sourcetypes):
    """Get the command to link the given source."""
    if 'c++' in sourcetypes or 'objc++' in sourcetypes:
        cc = 'CXX'
    else:
        cc = 'CC'

    cmd = [variables.get(variant, cc)]
    cmd.extend(variables.get(variant, 'LDFLAGS', ()))
    cmd.extend(('-o', output))
    cmd.extend(sources)
    cmd.extend(variables.get(variant, 'LIBS', ()))
    return cmd

class GnuMakeModule(Module):
    """A module for the GNU Make target."""
    __slots__ = []

    def flag_variable(self, flag):
        """Get the variable name that correspond to a flag."""
        raise NotImplementedError('must be implemented by subclass')

    def add_define(self, definition, *, configs=None, archs=None):
        """Add a preprocessor definition."""
        var = {'CPPFLAGS:': ['-D' + definition]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_header_path(self, path, *,
                        configs=None, archs=None, system=False):
        """Add a header search path.

        If system is True, then the header path will be searched for
        include statements with angle brackets.  Otherwise, the header
        path will be searched for include statements with double
        quotes.  Not all targets support the distinction.
        """
        flag = '-I' if system else '-iquote'
        var = {'CPPFLAGS': [flag + path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_library(self, path, *, configs=None, archs=None):
        """Add a library."""
        var = {'LIBS': [path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_library_path(self, path, *, configs=None, archs=None):
        """Add a library search path."""
        var = {'LDFLAGS': ['-L' + path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def test_compile(self, source, sourcetype, *,
                     configs=None, archs=None, external=True, link=True):
        """Try to compile a source file.

        Returns True if successful, False otherwise.
        """
        log = logfile(2)
        print('Testing compilation.', file=log)
        print('Source type:', sourcetype, file=log)
        log.write(format_block(source))
        variables = self.variables()
        with tempfile.TemporaryDirectory() as dirpath:
            file_c = os.path.join(dirpath, 'config' + SRCTYPE_EXT[sourcetype])
            file_obj = os.path.join(dirpath, 'config.o')
            file_out = os.path.join(dirpath, 'out')
            with open(file_c, 'w') as fp:
                fp.write(source)
            for variant in self.schema.get_variants(configs, archs):
                print('Variant:', variant, file=log)
                cmds = [cc_command(
                    variables, variant, file_obj, file_c,
                    sourcetype, external=external)]
                if link:
                    cmds.append(ld_command(
                        variables, variant, file_out, [file_obj],
                        [sourcetype]))
                for cmd in cmds:
                    print('Command:', escape_cmd(cmd), file=log)
                    try:
                        stdout, retcode = get_output(cmd, combine_output=True)
                    except ConfigError as ex:
                        print(ex, file=log)
                        return False
                    log.write(format_block(stdout))
                    if retcode != 0:
                        print('Command failed.', file=log)
                        return False
        return True
