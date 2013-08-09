import build.shell as shell
from build.object import env
from build.error import ProjectError, ConfigError, format_block
from build.path import Path
import tempfile
import os
import io

class MakefileTarget(object):
    __slots__ = ['base_env', 'os']
    archs = None

    def __init__(self, subtarget, cfg, vars, archs):
        if archs is not None:
            raise ConfigError(
                'architecture cannot be specified for this target')
        self.base_env = default_env(cfg, vars, subtarget)
        fp = cfg.getfp(2)
        fp.write('Makefile environment:\n')
        env.dump_env(self.base_env, fp, indent='  ')
        self.os = subtarget

    def gen_build(self, cfg, proj):
        from .make import gmake
        from build.object.build import Build
        build = Build(cfg, proj, BUILDERS)
        makefile = gmake.Makefile(cfg)
        makepath = Path('/Makefile', 'builddir')
        makefile.add_build(build, makepath)
        makefile.add_clean(Path('/build', 'builddir'))
        return build

    @property
    def config_env(self):
        return self.base_env

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
    for framework in env.get('FRAMEWORKS', ()):
        cmd.extend(('-framework', framework))
    return cmd

def default_env(cfg, vars, osname):
    """Get the default environment."""
    assert osname in ('linux', 'osx')
    config = 'debug' if cfg.config is None else cfg.config
    warnings = True if cfg.warnings is None else cfg.warnings
    werror = (config == 'debug') if cfg.werror is None else cfg.werror
    
    base_env = {}
    envs = [base_env]
    if cfg.config == 'debug':
        cflags = ('-O0', '-g')
    else:
        cflags = ('-O2', '-g')
    base_env.update({
        'CC': 'cc',
        'CXX': 'c++',
        'CFLAGS': cflags,
        'CXXFLAGS': cflags,
    })
    if warnings:
        base_env.update({
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

    user_env = {}
    for varname in vars.items():
        try:
            varparse = env.VAR[varname].parse
        except (KeyError, AttributeError):
            continue
        user_env[varname] = varparse(vars[varname])
        try:
            del base_env[varname]
        except KeyError:
            pass
    envs.append(user_env)

    return env.merge_env(envs)

def getmachine(cfg, env):
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
            cfg.warn('unknown machine name: {}'.format(m))
    raise ConfigError(
        'unable to parse machine name',
        shell.describe_proc(cmd, stdout + stderr, retcode))

def parse_flags(mod, flag, value):
    ignored = frozenset(mod.info.get_string('ignore.' + flag, '').split())
    return tuple(f for f in value.split() if f not in ignored)

def build_pkg_config(build, mod, name, external):
    spec = mod.info.get_string('spec')
    cmdname = 'pkg-config'
    flags = {}
    for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
        stdout, stderr, retcode = shell.get_output(
            [cmdname, '--silence-errors', arg, spec])
        if retcode:
            stdout, retcode = shell.get_output(
                [cmdname, '--print-errors', '--exists', arg, spec],
                combine_output=True)
            raise ConfigError('{} failed'.format(cmdname), stdout)
        flags[varname] = parse_flags(mod, varname, stdout)
    flags['CXXFLAGS'] = flags['CFLAGS']
    build.add(name, env.EnvModule(flags))

def build_sdl_config(build, mod, name, external):
    min_version = mod.info.get_string('min-version')
    cmdname = 'sdl-config'
    flags = {}
    for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
        stdout, stderr, retcode = get_output([cmdname, arg])
        if retcode:
            raise ConfigError('{} failed'.format(cmdname), stderr)
        flags[varname] = parse_flags(mod, varname, stdout)
    flags['CXXFLAGS'] = flags['CFLAGS']
    build.add(name, env.EnvModule(flags))

def library_search(build, src_lang, src_prologue, src_body,
                   flagsets, name):
    log = io.StringIO()
    srctext = gen_source(src_prologue, src_body)
    log.write(format_block(srctext))
    if src_lang == 'c':
        filename = 'config.c'
    elif src_lang == 'c++':
        filename = 'config.cpp'
    else:
        raise ConfigError('unknown language: {!r}'.format(src_lang))

    base_env = build.cfg.target.config_env
    with tempfile.TemporaryDirectory() as tempdir:
        src = os.path.join(tempdir, filename)
        obj = os.path.join(tempdir, 'config.o')
        out = os.path.join(tempdir, 'config.out')
        with open(src, 'w', encoding='UTF-8') as fp:
            fp.write(srctext)
        for flags in flagsets:
            test_env = env.merge_env([base_env, flags])
            cmds = [cc_cmd(test_env, obj, src, src_lang),
                    ld_cmd(test_env, out, [obj], [src_lang])]
            for cmd in cmds:
                stdout, retcode = shell.get_output(
                    cmd, combine_output=True)
                log.write(shell.describe_proc(cmd, stdout, retcode))
                if retcode:
                    break
            else:
                build.add(name, env.EnvModule(flags))
                return
    raise ConfigError('could not link test file', log.getvalue())

def build_library_search(build, mod, name, external):
    src_lang = mod.info.get_string('source.language')
    src_prologue = mod.info.get_string('source.prologue', '')
    src_body = mod.info.get_string('source.body', '')
    flagsets = {}
    for k, v in mod.info.items():
        if not k.startswith('flags.'):
            continue
        try:
            flagnum, flagname = k.split('.', 2)[1:]
            flagnum = int(flagnum)
        except ValueError:
            raise ConfigError('invalid flag name: {!r}'
                              .format(k))
        if flagname not in ('CFLAGS', 'CXXFLAGS', 'LIBS'):
            raise ConfigError('unknown flag name: {!r}'
                              .format(flagname))
        flagsets.setdefault(flagnum, {})[flagname] = v
    flagsets = [{}] + [v for k, v in sorted(flagsets.items())]
    library_search(
        build, src_lang, src_prologue, src_body, flagsets, name)

def build_framework(build, mod, name, external):
    if build.cfg.target.os not in ('osx',):
        raise ConfigError(
            'frameworks not available on this platform')
    filename = mod.info.get_string('filename')
    library_search(
        build, 'c', '', '', [{'FRAMEWORKS': (filename,)}], name)

BUILDERS = {
    'pkg-config': build_pkg_config,
    'sdl-config': build_sdl_config,
    'library-search': build_library_search,
    'framework': build_framework,
}
