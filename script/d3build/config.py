# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import argparse
import platform
from .error import UserError, ConfigError
from .shell import escape

PLATFORMS = {
    'Darwin': ('osx', 'xcode'),
    'Linux': ('linux', 'gmake'),
    'Windows': ('windows', 'msvc'),
}

class Config(object):
    __slots__ = [
        # Output verbosity: 0, 1, or 2
        'verbosity',
        # The configuration: release or debug
        'config',
        # Whether to enable extra warnings
        'warnings',
        # Whether warnings are treated as errors
        'werror',
        # Variables specified on the configuration command line
        'variables',
        # The target platform
        'platform',
        # The target toolchain
        'target',
        # Target toolchain parameters
        'targetparams',
        # Optional flags from the command line
        'flags',
        # The identity of the script being run
        'script',
    ]

    @classmethod
    def argument_parser(class_, options):
        p = argparse.ArgumentParser()
        buildgroup = p.add_argument_group('build settings')
        optgroup = p.add_argument_group('features')

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

        def parse_yesno(name, *, dest=None, default=None, help=None,
                        help_neg=None):
            if dest is None:
                dest = name
            if help_neg is None:
                help_neg = argparse.SUPPRESS
            buildgroup.add_argument(
                '--' + name,
                default=default, action='store_true',
                dest=dest, help=help)
            buildgroup.add_argument(
                '--no-' + name,
                default=default, action='store_false',
                dest=dest, help=help_neg)

        buildgroup.add_argument(
            '--target',
            help='select a build target')
        buildgroup.add_argument(
            '--debug', dest='config', default='release',
            action='store_const', const='debug',
            help='use debug configuration')
        buildgroup.add_argument(
            '--release', dest='config', default='release',
            action='store_const', const='release',
            help='use release configuration')
        parse_yesno(
            'warnings',
            help='enable extra compiler warnings',
            help_neg='disable extra compiler warnings')
        parse_yesno(
            'werror',
            help='treat warnings as errors',
            help_neg='do not treat warnings as errors')

        for option in options:
            option.add_argument(optgroup)

        return p

    @classmethod
    def parse_config(class_, *, options=[], args, script):
        args = class_.argument_parser(options).parse_args(args)
        obj = class_()

        obj.verbosity = args.verbosity
        obj.config = 'release' if args.config is None else args.config
        obj.warnings = True if args.warnings is None else args.warnings
        obj.werror = (obj.config == 'debug'
                      if args.werror is None else args.werror)

        obj.variables = {}
        for vardef in args.variables:
            i = vardef.find('=')
            if i < 0:
                raise UserError(
                    'invalid variable syntax: {!r}'.format(vardef))
            obj.variables[vardef[:i]] = vardef[i+1:]

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

        obj.script = script

        return obj

    def dump(self):
        print('Platform:', self.platform)
        print('Target:', ':'.join(
            [self.target] +
            sorted('{}={}'.format(param, value)
                   for param, value in self.targetparams.items())))
        print('Configuration:', self.config)
        print('Extra warnings:', self.warnings)
        print('Warnings are errors:', self.werror)
        print('Build variables:')
        for varname, value in sorted(self.variables.items()):
            print('    {} = {}'.format(varname, value))

    def environment(self):
        if self.target == 'gmake':
            from .environment import gmake
            return gmake.GnuMakeEnvironment(self)
        else:
            raise ConfigError(
                'unknown target: {}'.format(self.target))

    @classmethod
    def run(class_, *, configure, sources, options=[], apply_defaults=None):
        import sys, pickle
        args = sys.argv
        action = args[1] if len(args) >= 2 else None
        if action == '--action-regenerate':
            with open('config.dat', 'rb') as fp:
                _, gensources = pickle.load(fp)
            if len(args) != 3:
                print('Invalid arguments', file=sys.stderr)
                sys.exit(1)
            try:
                source = gensources[args[2]]
            except KeyErorr:
                print('Unknown generated source file: {}'.format(args[2]))
                sys.exit(1)
            source = pickle.loads(source)
            source.regen()
            return
        elif action == '--action-reconfigure':
            with open('config.dat', 'rb') as fp:
                args, gensources = pickle.load(fp)
        cfg = class_.parse_config(options=options, args=args[1:],
                                  script=sys.argv[0])
        if apply_defaults is not None:
            apply_defaults(cfg)
        env = cfg.environment()
        print('Arguments: {}'.format(
            ' '.join(escape(x) for x in args)), file=env.logfile())
        env.log_info()
        configure(env)
        env.finalize()
        gensources = {
            s.target: pickle.dumps(s, protocol=pickle.HIGHEST_PROTOCOL)
            for s in env.generated_sources
        }
        with open('config.dat', 'wb') as fp:
            pickle.dump((args, gensources), fp)

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
