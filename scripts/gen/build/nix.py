from gen.error import ConfigError

def mkdef(k, v):
    if v is None:
        return '-D{}'.format(k)
    return '-D{}={}'.format(k, v)

def cc_cmd(env, output, source, sourcetype, depfile=None):
    """Get the command to compile the given source."""
    if sourcetype in ('c', 'm'):
        try:
            cc = env['CC']
        except KeyError:
            raise ConfigError('CC is not set')
        cflags = env.get('CFLAGS', ())
        warn = env.get('CWARN', ())
    elif sourcetype in ('cxx', 'mm'):
        try:
            cc = env['CXX']
        except KeyError:
            raise ConfigError('CXX is not set')
        cflags = env.get('CXXFLAGS', ())
        warn = env.get('CXXWARN', ())
    else:
        assert False
    cmd = [cc, '-o', output.posix, source.posix, '-c']
    if depfile is not None:
        cmd.extend(('-MF', depfile.posix, '-MMD', '-MP'))
    cmd.extend('-I' + p.posix for p in env.get('CPPPATH', ()))
    cmd.extend('-F' + p for p in env.get('FPATH', ()))
    cmd.extend(mkdef(k, v) for k, v in env.get('DEFS', ()))
    cmd.extend(env.get('CPPFLAGS', ()))
    if not env.get('external', False):
        cmd.extend(warn)
    cmd.extend(cflags)
    return cmd

def ld_cmd(env, output, sources, sourcetypes):
    """Get the command to link the given source."""
    if 'cxx' in sourcetypes or 'mm' in sourcetypes:
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
    cmd.extend(('-o', output.posix))
    cmd.extend(s.posix for s in sources)
    cmd.extend('-F' + p for p in env.get('FPATH', ()))
    cmd.extend(env.get('LIBS', ()))
    return cmd
