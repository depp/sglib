# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from build.error import Error, ConfigError
from build.path import Path, split_native
import build.shell as shell
import collections
import argparse
import sys
import platform
import os
import importlib
import pickle
import io
import re
import datetime

TARGETS = {'make', 'msvc', 'xcode'}
OS = {'osx', 'linux', 'windows'}
PLATFORMS = {
    'Darwin': ('osx', 'xcode'),
    'Linux': ('linux', 'make'),
    'Windows': ('windows', 'msvc'),
}

EXISTS_CACHE = {}

class Tee(io.TextIOBase):
    __slots__ = ['files']
    def __init__(self, files):
        super(Tee, self).__init__()
        self.files = tuple(files)
    def writable(self):
        return True
    def write(self, s):
        for fp in self.files:
            fp.write(s)

def redirect_log(*, append):
    global logfile
    global console
    console = sys.stderr
    mode = 'a' if append else 'w'
    logfile = open('config.log', mode)
    sys.stderr = Tee([sys.stderr, logfile])

def parse_archs(x):
    archs = x.split()
    if not archs:
        raise ValueError('empty architecture list')
    arch_re = re.compile(r'^\w+$', re.ASCII)
    aset = set()
    for arch in archs:
        if arch in aset:
            raise ValueError('duplicat architecture: {}'.format(arch))
        if not arch_re.match(arch):
            raise ValueError('invalid architecture: {!r}'.format(arch))
        aset.add(arch)
    return tuple(archs)

class Config(object):
    """Result from reading user configuration options."""
    ARGS = ['bundled_libs', 'verbosity',
            'warnings', 'werror', 'config']
    __slots__ = ARGS + [
        'srcdir', 'projfile', 'target', 'flags',
        'srcdir_target', '_sep_target', 'verbosity']

    def __init__(self, srcdir, projfile, args):
        self.srcdir = srcdir
        self.projfile = projfile
        for arg in self.ARGS:
            setattr(self, arg, getattr(args, arg))

    def set_target(self, target, flags):
        self.target = target
        self.flags = flags

        if target.os == 'windows':
            import ntpath as path
        else:
            import posixpath as path

        if os.path is path:
            self.srcdir_target = self.srcdir
        else:
            parts = split_native(self.srcdir)
            self.srcdir_target = path.sep.join(parts)

        self._sep_target = path.sep

    def path(self, path, base='srcdir'):
        """Convert a native path to a path object."""
        if base == 'srcdir':
            bpath = self.srcdir
        elif base == 'builddir':
            bpath = os.path.curdir
        else:
            raise ValueError('invalid base: {}'.format(base))
        rel_path = os.path.relpath(path, bpath)
        drive, dpath = os.path.splitdrive(rel_path)
        try:
            if drive or os.path.isabs(dpath):
                raise ValueError(
                    'path not contained in ${{{}}}' .format(base))
            return Path('/', base).join(dpath.replace(os.path.sep, '/'))
        except ValueError as ex:
            raise ValueError('{}: {!r}'.format(ex, path))

    def native_path(self, path):
        """Convert a path to a native path."""
        ppath = path.path[1:].replace('/', os.path.sep) or os.path.curdir
        if path.base == 'srcdir':
            bpath = self.srcdir
        else:
            bpath = os.path.curdir
        if bpath == os.path.curdir:
            return ppath
        if ppath == os.path.curdir:
            return bpath
        return os.path.join(bpath, ppath)

    def target_path(self, path):
        """Convert a path to a target path."""
        sep = self._sep_target
        ppath = path.path[1:].replace('/', sep) or '.'
        if path.base == 'builddir':
            return ppath
        base = self.srcdir_target
        if base == '.':
            return ppath
        if ppath == '.':
            return base
        if base.endswith(sep):
            return base + ppath
        return base + sep + ppath

    def getfp(self, level):
        """Get logging file object for the given log level."""
        if self.verbosity >= level:
            return sys.stderr
        return logfile

    def warn(self, msg, extra=None):
        fp = self.getfp(1)
        fp.write('warning: {}\n'.format(msg))
        if extra is not None:
            fp.write(extra)
            if not extra.endswith('\n'):
                fp.write('\n')

    def exists(self, path):
        ec = EXISTS_CACHE
        try:
            return ec[path]
        except KeyError:
            pass
        result = os.path.exists(self.native_path(path))
        ec[path] = result
        return result

Value = collections.namedtuple('Value', 'name help')

class ConfigTool(object):
    __slots__ = ['parser', 'flaginfo', 'optgroup', 'defaults',
                 'paths']

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
        self.paths = []

        self.parser.add_argument('var', nargs='*',
                                 help='build variables, VAR=VALUE')

        self.parser.add_argument(
            '-q', dest='verbosity', default=1,
            action='store_const', const=0,
            help='suppress messages')
        self.parser.add_argument(
            '-v', dest='verbosity', default=1,
            action='store_const', const=2,
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
        buildgroup.add_argument(
            '--with-bundled-libs', dest='bundled_libs', default='auto',
            action='store_const', const='yes',
            help='always use bundled libraries')
        buildgroup.add_argument(
            '--without-bundled-libs', dest='bundled_libs', default='auto',
            action='store_const', const='no',
            help='never use bundled libraries')
        buildgroup.add_argument(
            '--archs', dest='archs', default=None, type=parse_archs,
            help='specify build architectures')

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

    def add_path(self, path):
        self.paths.append(path)

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

    def get_target(self, cfg, args):
        osname, default_target = PLATFORMS.get(
            chrplatform.system(), (None, None))
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
        from . import varlist
        vars = varlist.VarList.parse(args.var)
        target = mod.target(subtarget, osname, cfg, vars, args.archs)
        for varname in vars.unused():
            cfg.warn('unknown variable: {!r}'.format(varname))
        return target

    def action_config(self):
        if len(sys.argv) < 3:
            print('invalid usage', file=sys.stderr)
            sys.exit(1)
        redirect_log(append=False)
        logfile.write(
            'Configure {}\n'.format(datetime.datetime.now().isoformat()))
        sys.stderr.write(
            'Arguments: {}\n' .format(shell.escape_cmd(sys.argv[3:])))

        projfile = sys.argv[2]
        srcdir = os.path.dirname(projfile) or os.path.curdir
        projfile = os.path.basename(projfile)
        if not projfile:
            raise ConfigError('not a project file: {!r}'
                              .format(sys.argv[2]))
        projfile = Path('/', 'srcdir').join1(projfile)

        args = self.parser.parse_args(sys.argv[3:])
        cfg = Config(srcdir, projfile, args)

        target = self.get_target(cfg, args)

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

        cfg.set_target(target, flags)

        if cfg.verbosity >= 1:
            self.dump_flags(flags)
            if target.archs is not None:
                print('Build architectures: {}'
                      .format(', '.join(target.archs)),
                      file=sys.stderr)

        self.run_config(cfg, args)

    def get_cache(self):
        with open('config.dat', 'rb') as fp:
            return pickle.load(fp)

    def action_reconfig(self):
        redirect_log(append=True)
        logfile.write('\n{}\nReconfigure {}\n'.format(
            '#' * 40, datetime.datetime.now().isoformat()))
        self.run_config(pickle.loads(self.get_cache()[0]), None)

    def build(self, targetpath, target, cfg):
        fp = io.StringIO()
        target.write(fp, cfg)
        text = fp.getvalue()
        if target.is_regenerated_always:
            del fp
            try:
                with open(targetpath, 'r', encoding='UTF-8') as fp:
                    curtext = fp.read()
            except EnvironmentError:
                pass
            else:
                if curtext == text:
                    return
        if os.path.dirname(targetpath):
            os.makedirs(os.path.dirname(targetpath), exist_ok=True)
        with open(targetpath, 'w', encoding='UTF-8') as fp:
            fp.write(text)

    def run_config(self, cfg, args):
        from . import project
        paths = [cfg.path(p) for p in self.paths]
        proj = project.Project.load_xml(cfg, paths)
        if args and args.dump_project:
            self.dump_project(proj)
            sys.exit(0)
        build = cfg.target.gen_build(cfg, proj)

        cache_gen_sources = {}
        all_gen_sources = []
        gen_paths = set()
        for target in build.generated_sources():
            if target.target in gen_paths:
                raise ProjectError(
                    'multiple generated sources with same path: {}'
                    .format(target.target))
            gen_paths.add(target.target)
            if target.is_regenerated:
                cache_gen_sources[cfg.native_path(target.target)] = target
            if not target.is_regenerated_only:
                all_gen_sources.append(target)

        cache_obj = (
            pickle.dumps(cfg, protocol=pickle.HIGHEST_PROTOCOL),
            pickle.dumps(cache_gen_sources,
                         protocol=pickle.HIGHEST_PROTOCOL),
        )
        with open('config.dat', 'wb') as fp:
            pickle.dump(cache_obj, fp, protocol=pickle.HIGHEST_PROTOCOL)

        for target in all_gen_sources:
            targetpath = cfg.native_path(target.target)
            if cfg.verbosity >= 1:
                print('Writing {}'.format(targetpath), file=sys.stderr)
            self.build(targetpath, target, cfg)

    def action_regen(self):
        if len(sys.argv) < 3:
            print('invalid usage', file=sys.stderr)
            sys.exit(1)
        cache = self.get_cache()
        cfg = pickle.loads(cache[0])
        cache_gen_sources = pickle.loads(cache[1])
        targetpath = sys.argv[2]
        self.build(targetpath, cache_gen_sources[targetpath], cfg)

    def _run(self):
        if len(sys.argv) < 2:
            print('invalid usage', file=sys.stderr)
            sys.exit(1)
        action = sys.argv[1]
        try:
            func = getattr(self, 'action_{}'.format(action))
        except AttributeError:
            print('unknown action: {}'.format(action), file=sys.stderr)
            sys.exit(1)
        func()

    def run(self):
        try:
            self._run()
        except Error as ex:
            ex.write(sys.stderr)
            sys.exit(1)
