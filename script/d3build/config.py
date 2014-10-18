import argparse
import platform
from .error import UserError, ConfigError

PLATFORMS = {
    'Darwin': ('osx', 'xcode'),
    'Linux': ('linux', 'make'),
    'Windows': ('windows', 'msvc'),
}

class Config(object):
    __slots__ = [
        'config', 'warnings', 'werror', 'variables',
        'platform', 'target', 'targetparams',
        'flags',
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
    def configure(class_, *, options=[]):
        args = class_.argument_parser(options).parse_args()
        obj = class_()

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
