# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import argparse
import platform
import sys
from .error import UserError, ConfigError
from .shell import escape
from .util import yesno

PLATFORMS = {
    'Darwin': ('osx', 'xcode'),
    'Linux': ('linux', 'gnumake'),
    'Windows': ('windows', 'msvc'),
}

class Config(object):
    """Project configuration."""
    __slots__ = [
        # Output verbosity: 0, 1, or 2
        'verbosity',
        # List of variables specified on the configuration command line
        'variables',
        # The name of the target platform
        'platform',
        # The name of the target toolchain
        'target',
        # Target toolchain parameters
        'targetparams',
        # Dictionary mapping flag names to values
        'flags',
    ]

    @classmethod
    def argument_parser(class_, options):
        """Get the argument parser."""
        p = argparse.ArgumentParser()

        p.add_argument('variables', nargs='*', metavar='VAR=VALUE',
                       help='build variables')
        p.add_argument(
            '-q', dest='verbosity', default=1,
            action='store_const', const=0,
            help='suppress messages')
        p.add_argument(
            '-v', dest='verbosity', default=1,
            action='store_const', const=2,
            help='verbose messages')
        p.add_argument(
            '--target',
            help='select a build target')

        optgroup = p.add_argument_group('features')
        for option in options:
            option.add_argument(optgroup)

        return p

    @classmethod
    def parse_config(class_, *, options=[], args):
        """Parse the configuration from command-line arguments."""
        args = class_.argument_parser(options).parse_args(args)
        obj = class_()

        obj.verbosity = args.verbosity
        obj.variables = args.variables

        plat = platform.system()
        try:
            obj.platform, target = PLATFORMS[plat]
        except KeyError:
            raise ConfigError('unknown platform: {}'.format(plat))
        if args.target is not None:
            target = args.target
        parts = target.split(':')
        obj.targetparams = {}
        for part in parts[1:]:
            i = part.find('=')
            if i < 0:
                raise UserError(
                    'invalid target parameter: {!r}'.format(part))
            obj.targetparams[part[:i]] = part[i+1:]
        obj.target = parts[0]

        obj.flags = {}
        for option in options:
            obj.flags[option.name] = option.get_value(args)

        return obj

    def dump(self, *, file=None):
        """Dump information about the configuration."""
        if file is None:
            file = sys.stdout
        print('Platform:', self.platform, file=file)
        print('Target:', ':'.join(
            [self.target] +
            sorted('{}={}'.format(param, value)
                   for param, value in self.targetparams.items())),
            file=file)
        print('Build variables:', file=file)
        for variable in self.variables:
            print('  {}'.format(variable), file=file)
        print('Flags:', file=file)
        for k, v in sorted(self.flags.items()):
            if isinstance(v, bool):
                v = yesno(v)
            print('  {}: {}'.format(k, v), file=file)

if __name__ == '__main__':
    cfg = Config.configure()
    cfg.dump()
    from .environment import nix
    from .environment.variable import BuildVariables
    env = nix.NixEnvironment(cfg)
    print('Environment build variables:')
    env.base_vars.dump(indent='    ')
    print('CC command:', ' '.join(nix.cc_command(
        env.base_vars, 'output.o', 'input.c', 'c', depfile='output.d')))
    print('LD command:', ' '.join(nix.ld_command(
        env.base_vars, 'output', ['file1.o', 'file2.o'], {'c'})))
    source = """\
#include <stdio.h>
int main(int argc, char **argv)
{
    return 0;
}
"""
    v = env.test_compile_link(source, 'c', env.base_vars, [
        BuildVariables(CC='gcc-4.2'),
        BuildVariables(CC='gcc'),
    ])
    print("GOT IT:")
    v.dump(indent='    ')
