import collections
import argparse
import sys
import platform
import build.project
import os
import importlib
from build.error import ConfigError

TARGETS = {'make', 'msvc', 'xcode'}
OS = {'osx', 'linux', 'windows'}
PLATFORMS = {
    'Darwin': ('osx', 'xcode'),
    'Linux': ('linux', 'make'),
    'Windows': ('windows', 'msvc'),
}

Value = collections.namedtuple('Value', 'name help')

class Config(object):
    __slots__ = ['parser', 'flaginfo', 'optgroup', 'defaults']

    def __init__(self, prog='config.sh',
                 description='Configure the build system'):
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
        self.parser.add_argument(
            '--dump-project', action='store_true')

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
            '--target',
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
        print('Configuration:', file=sys.stderr)
        for k, v in sorted(flags.items()):
            print('  {}: {}'.format(k, v), file=sys.stderr)

    def dump_project(self, proj):
        import sys
        import io
        fp = io.BytesIO()
        proj.dump_xml(fp)
        sys.stdout.write(fp.getvalue().decode('utf-8'))

    def get_target(self, args):
        osname, default_target = PLATFORMS.get(platform.system(), (None, None))
        target = args.target
        if target is None:
            target = default_target
        if target is None:
            raise ConfigError('unknown platform, no default target: {}'
                              .format(platform.system()))
        i = target.find(':')
        if i >= 0:
            subtarget = target[i+1:]
            target = target[:i]
        else:
            subtarget = None
        if target not in TARGETS:
            raise ConfigError('invalid target: {!r}'.format(target))
        mod = importlib.import_module('build.target.' + target)
        return mod.Target(subtarget, osname, args)

    def _run(self):
        if len(sys.argv) < 2:
            print('invalid usage', file=sys.stderr)
            sys.exit(1)
        projfile = sys.argv[1]
        args = self.parser.parse_args(sys.argv[2:])

        target = self.get_target(args)

        flags = {}
        for flag in self.flaginfo:
            val = getattr(args, 'flag:' + flag)
            if val is None:
                val = 'no'
                for target_os in (target.os, None):
                    try:
                        val = self.defaults[target_os][flag]
                        break
                    except KeyError:
                        pass
            flags[flag] = val

        if args.verbosity >= 1:
            self.dump_flags(flags)

        environ = {
            'os':target.os,
            'flag':flags,
        }
        srcdir = os.path.dirname(projfile) or os.path.curdir
        proj = build.project.Project(srcdir, srcdir, environ)
        proj.load_xml(projfile)
        if args.dump_project:
            self.dump_project(proj)
            sys.exit(0)
        target.build(proj)

    def run(self):
        try:
            self._run()
        except ConfigError as ex:
            ex.write(sys.stderr)
            sys.exit(1)
