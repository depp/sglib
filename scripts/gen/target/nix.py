from gen.target.config import TargetConfig
from gen.error import ConfigError, format_block
from gen.build.nix import cc_cmd, ld_cmd
from gen.shell import get_output, describe_proc
from gen.path import Path
from gen.env import parse_env, merge_env
import os
import io

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

def cleanup_test():
    for fname in os.listdir('.'):
        if fname.startswith('config.tmp'):
            os.unlink(fname)

class NixConfig(TargetConfig):
    __slots__ = TargetConfig.__slots__ + ['test_env']

    def __init__(self, opts, user_env, os, project):
        base_env = merge_env([default_env(opts, os), user_env])
        super(NixConfig, self).__init__(opts, base_env, os, project)
        self.test_env = dict(base_env)
        self.test_env['external'] = True

    def config_pkgconfig(self, spec):
        name = 'pkg-config'
        env = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = get_output(
                [name, '--silence-errors', arg, spec])
            if retcode:
                stdout, retcode = get_output(
                    [name, '--print-errors', '--exists', spec],
                    combine_output=True)
                raise ConfigError('{} failed'.format(name), stdout)
            env[varname] = tuple(stdout.split())
        return env

    def config_sdlconfig(self, version):
        name = 'sdl-config'
        env = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = get_output([name, arg])
            if retcode:
                # FIXME failed should return code
                raise ConfigError('{} failed'.format(name), stderr)
            env[varname] = tuple(stdout.split())
        return env

    def libsearch(self, prologue, body, envs):
        src = Path('config.tmp.c')
        obj = Path('config.tmp.o')
        out = Path('config.tmp.out')
        log = io.StringIO()
        sourcetext = gen_source(prologue, body)
        log.write('Test file:\n')
        log.write(format_block(sourcetext))
        try:
            with open('config.tmp.c', 'w') as fp:
                fp.write(sourcetext)
            for env in envs:
                test_env = merge_env([self.test_env, env])
                cmd1 = cc_cmd(test_env, obj, src, 'c')
                cmd2 = ld_cmd(test_env, out, [obj], ['c'])
                for cmd in (cmd1, cmd2):
                    stdout, retcode = get_output(cmd, combine_output=True)
                    log.write(describe_proc(cmd, stdout, retcode))
                    if retcode:
                        break
                else:
                    return env
        finally:
            cleanup_test()
        raise ConfigError('could not link test file', log.getvalue())

    def config_framework(self, name):
        if self.os != 'osx':
            raise ConfigError('frameworks not available on this platform')
        return self.libsearch('', '', [{'LIBS': ('-framework', name)}])

    def config_librarysearch(self, source, flags):
        envs = [{}]
        envs.extend(flags)
        return self.libsearch(source.prologue, source.body, envs)

def default_env(opts, os):
    """Get the default environment."""
    assert os in ('linux', 'osx')
    config = 'debug' if opts.config is None else opts.config
    warnings = True if opts.warnings is None else opts.warnings
    werror = (config == 'debug') if opts.werror is None else opts.werror
    
    envs = []
    if opts.config == 'debug':
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
    if os == 'linux':
        envs.append({
            'LDFLAGS': ('-Wl,--as-needed', '-Wl,--gc-sections'),
        })
    if os == 'osx':
        envs.append({
            'LDFLAGS': ('-Wl,-dead_strip', '-Wl,-dead_strip_dylibs'),
        })
    return merge_env(envs)

def getmachine(env):
    """Get the name of the target machine."""
    cc = env['CC']
    cmd = [cc, '-dumpmachine']
    cmd.extend(env.get('CPPFLAGS', ()))
    cmd.extend(env.get('CFLAGS', ()))
    stdout, stderr, retcode = get_output(cmd)
    if retcode:
        raise ConfigError(
            'could not get machine specs',
            describe_proc(cmd, stderr, retcode))
    m = stdout.splitlines()
    if m:
        m = m[0]
        i = m.find('-')
        if i >= 0:
            m = m[:i]
            if m == 'x86_64':
                return 'x64'
            if re.match('i\d86', m):
                return 'x86'
            if m == 'powerpc':
                return 'ppc'
            sys.stderr.write(
                'warning: unknown machine name: {}'.format(m))
    raise ConfigError(
        'unable to parse machine name',
        describe_proc(cmd, stdout + stderr, retcode))
