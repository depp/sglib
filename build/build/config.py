import collections
import argparse
import sys
import platform

TARGETS = {}
def target(*names):
    def func(x):
        for name in names:
            TARGETS[name] = x
        return x
    return func

@target('make')
class MakeTarget(object):
    def __init__(self, args):
        system = platform.system()
        try:
            self.os = PLATFORMS[system]
        except KeyError:
            print('unsupported platform for makefile targets: {}'
                  .format(system), file=sys.stderr)
            sys.exit(1)

@target('xcode')
class XcodeTarget(object):
    def __init__(self, args):
        pass

    os = 'osx'

@target('msvc')
class MSVCTarget(object):
    def __init__(self, args):
        pass

    os = 'windows'

OS = {'osx', 'linux', 'windows'}

PLATFORMS = {
    'Darwin': 'osx',
    'Linux': 'linux',
    'Windows': 'windows',
}

DEFAULT_TARGET = {
    'Darwin': 'xcode',
    'Linux': 'make',
    'Windows': 'msvc',
}

Value = collections.namedtuple('Value', 'name help')

class Config(object):
    __slots__ = ['parser', 'flaginfo', 'optgroup', 'defaults']

    def __init__(self, prog='config.sh',
                 description='Configure the build system'):
        default_target = DEFAULT_TARGET.get(platform.system())

        self.parser = argparse.ArgumentParser(
            prog=prog,
            description=description)
        self.flaginfo = {}
        buildgroup = self.parser.add_argument_group(
            'build settings')
        self.optgroup = self.parser.add_argument_group(
            'optional features')
        self.defaults = {}

        self.parser.add_argument(
            '-q', dest='verbosity', default=1,
            action='store_const', const=0,
            help='suppress messages')
        self.parser.add_argument(
            '-v', dest='verbosity', default=1,
            action='store_const', const=0,
            help='verbose messages')

        buildgroup.add_argument(
            '--debug', dest='config', default='release',
            action='store_const', const='debug',
            help='use debug configuration')
        buildgroup.add_argument(
            '--release', dest='config', default='release',
            action='store_const', const='release',
            help='use release configuration')

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

        parse_yesno(
            'warnings',
            help='enable extra compiler warnings',
            help_neg='disable extra compiler warnings')
        parse_yesno(
            'werror',
            help='treat warnings as errors',
            help_neg='do not treat warnings as errors')

        buildgroup.add_argument(
            '--target', default=default_target,
            choices=list(sorted(TARGETS)),
            help='select a build target')

    def add_enable(self, name, help, values=None):
        if name in self.flaginfo:
            raise ValueError('duplicate flag')
        self.flaginfo[name] = values
        flage = '--enable-{}'.format(name)
        flagd = '--disable-{}'.format(name)
        dest = 'flag:{}'.format(name)
        if values is not None:
            self.optgroup.add_argument(
                flage, choices=[v.name for v in values],
                dest=dest, help=help)
        else:
            self.optgroup.add_argument(
                flage, action='store_const', const='yes',
                dest=dest, help=help)
        self.optgroup.add_argument(
            flagd, action='store_const', const='no',
            dest=dest, help=argparse.SUPPRESS)

    def add_defaults(self, defaults):
        for k, v in defaults.items():
            if k is not None and k not in OS:
                raise ValueError('unknown OS')
            for kk, vv in v.items():
                try:
                    values = self.flaginfo[kk]
                except KeyError:
                    raise ValueError('unknown flag: {!r}'.format(kk))
                if values is None:
                    values = ('yes', 'no')
                else:
                    values = [v.name for v in values] + ['no']
                if vv not in values:
                    raise ValueError('invalid value: {!r}'.format(vv))
            self.defaults.setdefault(k, {}).update(v)

    def dump_flags(self, flags):
        print('Configuration:')
        for k, v in sorted(flags.items()):
            print('  {}: {}'.format(k, v))

    def run(self):
        if len(sys.argv) < 2:
            print('invalid usage', file=sys.stderr)
            sys.exit(1)
        projfile = sys.argv[1]
        args = self.parser.parse_args(sys.argv[2:])

        if args.target is None:
            print('error: no default target for platform: {}'
                  .format(platform.system()), file=sys.stderr)
            sys.exit(1)
        target = TARGETS[args.target](args)

        flags = {}
        for flag in self.flaginfo:
            val = getattr(args, 'flag:' + flag)
            if val is None:
                val = 'no'
                for os in (target.os, None):
                    try:
                        val = self.defaults[os][flag]
                        break
                    except KeyError:
                        pass
            flags[flag] = val

        if args.verbosity >= 1:
            self.dump_flags(flags)
