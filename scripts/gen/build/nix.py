
def mkdef(k, v):
    if v is None:
        return '-D%s' % (k,)
    return '-D%s=%s' % (k, v)

def cc_cmd(env, output, source, sourcetype, depfile=None):
    """Get the command to compile the given source."""
    if sourcetype in ('c', 'm'):
        cc = env['CC']
        cflags = env['CFLAGS']
        warn = env['CWARN']
    elif sourcetype in ('cxx', 'mm'):
        cc = env['CXX']
        cflags = env['CXXFLAGS']
        warn = env['CXXWARN']
    else:
        assert False
    cmd = [cc, '-o', output.posix, source.posix, '-c']
    if depfile is not None:
        cmd.extend(('-MF', depfile.posix, '-MMD', '-MP'))
    cmd.extend('-I' + p.posix for p in env['CPPPATH'])
    cmd.extend(mkdef(k, v) for k, v in env['DEFS'])
    cmd.extend(env['CPPFLAGS'])
    if not env['external']:
        cmd.extend(warn)
    cmd.extend(cflags)
    return cmd

def ld_cmd(env, output, sources, sourcetypes):
    """Get the command to link the given source."""
    if 'cxx' in sourcetypes or 'mm' in sourcetypes:
        cc = env['CXX']
    else:
        cc = env['CC']
    cmd = [cc]
    cmd.extend(env['LDFLAGS'])
    cmd.extend(('-o', output.posix))
    cmd.extend(s.posix for s in sources)
    cmd.extend(env['LIBS'])
    return cmd
