from gen.error import ConfigError
from gen.build.nix import cc_cmd, ld_cmd
from gen.shell import getproc
from gen.path import Path
from gen.env.env import merge_env, MergeEnvironment
import subprocess
import platform
import os
from cStringIO import StringIO

def gen_source(prologue, body):
    fp = StringIO()
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

class NixConfig(object):
    __slots__ = ['test_env']

    def __init__(self, test_env):
        self.test_env = dict(test_env)
        self.test_env['external'] = True

    def config_pkgconfig(self, obj):
        name = 'pkg-config'
        exe = getproc(name)
        if exe is None:
            raise ConfigError('could not find %s' % name)
        env = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            cmd = [name, '--silence-errors', arg, obj.spec]
            proc = subprocess.Popen(
                cmd, stdout=subprocess.PIPE, executable=exe)
            stdout, stderr = proc.communicate()
            if proc.returncode:
                cmd = [name, '--print-errors', '--exists', obj.spec]
                proc = subprocess.Popen(
                    cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    executable=exe)
                stdout, stderr = proc.communicate()
                raise ConfigError('%s failed' % name, stdout)
            env[varname] = tuple(stdout.split())
        return env

    def config_sdlconfig(self, obj):
        name = 'sdl-config'
        exe = getproc(name)
        if exe is None:
            raise ConfigError('could not find %s' % name)
        env = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            proc = subprocess.Popen(
                [name, arg], stdout=subprocess.PIPE, executable=exe)
            stdout, stderr = proc.communicate()
            if proc.returncode:
                raise ConfigError('%s failed' % name)
            env[varname] = tuple(stdout.split())
        return env

    def libsearch(self, prologue, body, envs):
        src = Path('config.tmp.c')
        obj = Path('config.tmp.o')
        out = Path('config.tmp.out')
        log = StringIO()
        sourcetext = gen_source(prologue, body)
        log.write('Test file:\n')
        for line in sourcetext.splitlines():
            log.write('  | %s\n' % line)
        try:
            with open('config.tmp.c', 'w') as fp:
                fp.write(sourcetext)
            for env in envs:
                test_env = MergeEnvironment([self.test_env, env])
                cmd1 = cc_cmd(test_env, obj, src, 'c')
                cmd2 = ld_cmd(test_env, out, [obj], ['c'])
                for cmd in (cmd1, cmd2):
                    log.write('Command: %s\n' % ' '.join(cmd))
                    exe = getproc(cmd[0])
                    proc = subprocess.Popen(
                        cmd, executable=exe,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT)
                    stdout, stderr = proc.communicate()
                    log.write('Returned code %d\n' % proc.returncode)
                    log.write('Command output:\n')
                    for line in stdout.splitlines():
                        log.write('  | %s\n' % line)
                    if proc.returncode:
                        break
                else:
                    return env
        finally:
            cleanup_test()
        raise ConfigError('could not link test file', log.getvalue())

    def config_framework(self, obj):
        if platform.system() != 'Darwin':
            raise ConfigError('frameworks not available on this platform')
        return self.libsearch('', '', [{'LIBS': ('-framework', obj.name)}])

    def config_librarysearch(self, obj):
        return self.libsearch(
            obj.source.prologue,
            obj.source.body,
            envs = [{}] + obj.flags)

    def __call__(self, obj):
        return getattr(self, 'config_' + obj.srcname)(obj)

def default_env(config, os):
    """Get the default environment."""
    assert os in ('LINUX', 'OSX')
    envs = []
    if config.opts['config'] == 'debug':
        cflags = ('-O0', '-g')
    else:
        cflags = ('-O2', '-g')
    envs.append({
        'CC': 'cc',
        'CXX': 'c++',
        'CFLAGS': cflags,
        'CXXFLAGS': cflags,
    })
    if config.opts.get('warnings', True):
        envs.append({
            'CWARN': tuple(
                '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                '-Wwrite-strings -Wmissing-prototypes '
                .split()),
            'CXXWARN': tuple(
                '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                .split()),
        })
    if config.opts.get('werror', config.opts['config'] == 'debug'):
        envs.append({
            'CWARN': ('-Werror',),
            'CXXWARN': ('-Werror',),
        })
    if os == 'LINUX':
        envs.append({
            'LDFLAGS': ('-Wl,--as-needed', '-Wl,--gc-sections'),
        })
    if os == 'OSX':
        envs.append({
            'LDFLAGS': ('-Wl,-dead_strip', '-Wl,-dead_strip_dylibs'),
        })
    return merge_env(envs)
