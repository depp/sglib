import build.target as target
import build.data as data
import build.shell as shell
from . import env
from build.error import ConfigError, format_block
import tempfile
import os
import io

class EnvModule(object):
    __slots__ = ['name', 'env']
    is_target = False

    def __init__(self, name, env):
        self.name = name
        self.env = env

def gen_source(prologue, body):
    fp = io.StringIO()
    for line in prologue.splitlines():
        fp.write(line + '\n')
    fp.write(
        'int main(int argc, char *argv[])\n'
        '{\n')
    for line in body.splitlines():
        fp.write(line + '\n')
    fp.write(
        '    (void) argc;\n'
        '    (void) argv;\n'
        '    return 0;\n'
        '}\n')
    return fp.getvalue()

def mod_info_only(mod):
    if mod.group:
        raise ConfigError('module must be empty: {}'.format(mod.type))
    return mod.info

def cc_cmd(env, output, source, sourcetype, *, depfile=None, external=False):
    """Get the command to compile the given source."""
    if sourcetype in ('c', 'objc'):
        try:
            cc = env['CC']
        except KeyError:
            raise ConfigError('CC is not set')
        cflags = env.get('CFLAGS', ())
        warn = env.get('CWARN', ())
    elif sourcetype in ('c++', 'objc++'):
        try:
            cc = env['CXX']
        except KeyError:
            raise ConfigError('CXX is not set')
        cflags = env.get('CXXFLAGS', ())
        warn = env.get('CXXWARN', ())
    else:
        assert False
    cmd = [cc, '-o', output, source, '-c']
    if depfile is not None:
        cmd.extend(('-MF', depfile, '-MMD', '-MP'))
    cmd.extend(('-I', p) for p in env.get('CPPPATH', ()))
    cmd.extend(('-F', p) for p in env.get('FPATH', ()))
    cmd.extend(mkdef(k, v) for k, v in env.get('DEFS', ()))
    cmd.extend(env.get('CPPFLAGS', ()))
    if not external:
        cmd.extend(warn)
    cmd.extend(cflags)
    return cmd

def ld_cmd(env, output, sources, sourcetypes):
    """Get the command to link the given source."""
    if 'c++' in sourcetypes or 'objc++' in sourcetypes:
        try:
            cc = env['CXX']
        except KeyError:
            raise ConfigError('CXX is not set')
    else:
        try:
            cc = env['CC']
        except KeyError:
            raise ConfigError('CC is not set')
    cmd = [cc]
    cmd.extend(env.get('LDFLAGS', ()))
    cmd.extend(('-o', output))
    cmd.extend(sources)
    cmd.extend('-F' + p for p in env.get('FPATH', ()))
    cmd.extend(env.get('LIBS', ()))
    return cmd

def default_env(args, osname):
    """Get the default environment."""
    assert osname in ('linux', 'osx')
    config = 'debug' if args.config is None else args.config
    warnings = True if args.warnings is None else args.warnings
    werror = (config == 'debug') if args.werror is None else args.werror
    
    envs = []
    if args.config == 'debug':
        cflags = ('-O0', '-g')
    else:
        cflags = ('-O2', '-g')
    envs.append({
        'CC': 'cc',
        'CXX': 'c++',
        'CFLAGS': cflags,
        'CXXFLAGS': cflags,
    })
    if warnings:
        envs.append({
            'CWARN': tuple(
                '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                '-Wwrite-strings -Wmissing-prototypes '
                .split()),
            'CXXWARN': tuple(
                '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                .split()),
        })
    if werror:
        envs.append({
            'CWARN': ('-Werror',),
            'CXXWARN': ('-Werror',),
        })
    if osname == 'linux':
        envs.append({
            'LDFLAGS': ('-Wl,--as-needed', '-Wl,--gc-sections'),
        })
    if osname == 'osx':
        envs.append({
            'LDFLAGS': ('-Wl,-dead_strip', '-Wl,-dead_strip_dylibs'),
        })
    return env.merge_env(envs)

def getmachine(env):
    """Get the name of the target machine."""
    cc = env['CC']
    cmd = [cc, '-dumpmachine']
    cmd.extend(env.get('CPPFLAGS', ()))
    cmd.extend(env.get('CFLAGS', ()))
    stdout, stderr, retcode = shell.get_output(cmd)
    if retcode:
        raise ConfigError(
            'could not get machine specs',
            shell.describe_proc(cmd, stderr, retcode))
    m = stdout.splitlines()
    if m:
        m = m[0]
        i = m.find('-')
        if i >= 0:
            m = m[:i]
            if m == 'x86_64':
                return 'x64'
            if re.match(r'i\d86', m):
                return 'x86'
            if m == 'powerpc':
                return 'ppc'
            sys.stderr.write(
                'warning: unknown machine name: {}'.format(m))
    raise ConfigError(
        'unable to parse machine name',
        shell.describe_proc(cmd, stdout + stderr, retcode))

class Target(target.Target):
    __slots__ = ['env']

    def __init__(self, subtarget, osname, args):
        super(Target, self).__init__(subtarget, osname, args)
        self.env = default_env(args, osname)

    @property
    def os(self):
        return self.subtarget

    def mod_pkg_config(self, proj, mod, name):
        info = mod_info_only(mod)
        spec = info.get_string('spec')
        cmdname = 'pkg-config'
        flags = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = shell.get_output(
                [cmdname, '--silence-errors', arg, spec])
            if retcode:
                stdout, retcode = shell.get_output(
                    [cmdname, '--print-errors', '--exists', arg, spec],
                    combine_output = True)
                raise ConfigError('{} failed'.format(cmdname), stdout)
            flags[varname] = tuple(stdout.split())
        return [EnvModule(name, flags)]

    def mod_sdl_config(self, proj, mod, name):
        info = mod_info_only(mod)
        min_version = info.get_string('min-version')
        cmdname = 'sdl-config'
        flags = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = get_output([cmdname, arg])
            if retcode:
                raise ConfigError('{} failed'.format(cmdname), stderr)
            flags[varname] = tuple(stdout.split())
        return [EnvModule(name, flags)]

    def library_search(self, proj, src_lang, src_prologue, src_body,
                       flagsets, name):
        log = io.StringIO()
        srctext = gen_source(src_prologue, src_body)
        print('Test file:', file=log)
        log.write(format_block(srctext))
        if src_lang == 'c':
            filename = 'config.c'
        elif src_lang == 'c++':
            filename = 'config.cpp'
        else:
            raise ConfigError('unknown language: {!r}'.format(src_lang))

        with tempfile.TemporaryDirectory() as tempdir:
            src = os.path.join(tempdir, filename)
            obj = os.path.join(tempdir, 'config.o')
            out = os.path.join(tempdir, 'config.out')
            with open(src, 'w') as fp:
                fp.write(srctext)
            for flags in flagsets:
                test_env = env.merge_env([self.env, flags])
                cmds = [cc_cmd(test_env, obj, src, src_lang),
                        ld_cmd(test_env, out, [obj], [src_lang])]
                for cmd in cmds:
                    stdout, retcode = shell.get_output(
                        cmd, combine_output=True)
                    log.write(shell.describe_proc(cmd, stdout, retcode))
                    if retcode:
                        break
                else:
                    return [EnvModule(name, flags)]
        raise ConfigError('could not link test file', log.getvalue())

    def mod_library_search(self, proj, mod, name):
        info = mod_info_only(mod)
        src_lang = info.get_string('source.language')
        src_prologue = info.get_string('source.prologue', '')
        src_body = info.get_string('source.body', '')
        flagsets = {}
        for k, v in info.items():
            if not k.startswith('flags.'):
                continue
            try:
                flagnum, flagname = k.split('.', 2)[1:]
                flagnum = int(flagnum)
            except ValueError:
                raise ConfigError('invalid flag name: {!r}'
                                  .format(k))
            if flagname not in ('CFLAGS', 'LIBS'):
                raise ConfigError('unknown flag name: {!r}'
                                  .format(flagname))
            flagsets.setdefault(flagnum, {})[flagname] = v
        flagsets = [{}] + [v for k, v in sorted(flagsets.items())]
        return self.library_search(
            proj, src_lang, src_prologue, src_body, flagsets, name)

    def mod_framework(self, proj, mod, name):
        if self.subtarget not in ('osx',):
            raise ConfigError(
                'frameworks not available on this platform')
        info = mod_info_only(mod)
        filename = info.get_string('filename')
        return self.library_search(
            proj, 'c', '', '', [{'LIBS': ('-framework', filename)}], name)
