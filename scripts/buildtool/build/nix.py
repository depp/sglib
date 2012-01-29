import buildtool.path as path
import buildtool.shell as shell
import buildtool.build.target as target
from buildtool.env import Environment
import os

def customconfig(cmd):
    """Get the environment for a package from a config program."""
    if isinstance(cmd, str):
        cmd = [cmd]
    cflags = shell.getoutput(cmd + ['--cflags'])
    libs = shell.getoutput(cmd + ['--libs'])
    return Environment(CFLAGS=cflags, LIBS=libs)

def pkgconfig(pkg):
    """Get the environment for a package from pkg-config."""
    return customconfig(['pkg-config', pkg])

def getmachine(env):
    """Get the name of the target machine."""
    m = shell.getoutput([env.CC, '-dumpmachine'] +
                        env.CPPFLAGS + env.CFLAGS)
    m = m[:m.index('-')]
    return m

def buildline(cmd, target, tag):
    if tag is None:
        return '%s %s' % (cmd, target)
    else:
        return '[%s] %s %s' % (tag, cmd, target)

def getarch(env):
    aflags = []
    arch = env.ARCH
    if not arch:
        return None, aflags
    for a in arch:
        aflags.extend(('-arch', a))
    return ' '.join(arch), aflags

def cc(obj, src, env, stype):
    """Create a target that compiles C, C++, or Objective C. """
    if stype in ('c', 'm'):
        cc = env.CC
        cflags = env.CFLAGS
        warn = env.CWARN
        what = 'CC'
    elif stype in ('cxx', 'mm'):
        cc = env.CXX
        cflags = env.CXXFLAGS
        warn = env.CXXWARN
        what = 'CXX'
    else:
        raise ValueError('not a C file type: %r' % stype)
    tag, aflags = getarch(env)
    cmd = [cc, '-o', obj, '-c', src] + aflags + env.CPPFLAGS + warn + cflags
    return target.Target(cmd, inputs=[src], outputs=[obj],
                         quietmsg=buildline(what, src, tag))

def ld(obj, src, env):
    """Create a target that links an executable."""
    cc = env.CXX
    # Some LDFLAGS do not work if they don't appear before -o
    tag, aflags = getarch(env)
    cmd = [cc] + aflags + env.LDFLAGS + ['-o', obj] + src + env.LIBS
    return target.Target(cmd, inputs=src, outputs=[obj],
                         quietmsg=buildline('LD', obj, tag))

def lipo(obj, src, env):
    """Mac OS X: Create a universal executable from non-universal ones."""
    cmd = ['lipo'] + src + ['-create', '-output', obj]
    return target.Target(cmd, inputs=src, outputs=[obj],
                         quietmsg=buildline('LIPO', obj, None))

def env_nix(obj, **kw):
    # Base environment common to Mac OS X / Linux / etc

    # The user environment consists of defaults that can be overridden
    # by the user.  It always goes last, so user-specified flags can
    # override everything else.
    defaults = Environment(
        CC='gcc',
        CXX='g++',
        CPPFLAGS='',
        CFLAGS='-g -O2',
        CXXFLAGS='-g -O2',
        CWARN='-Wall -Wextra -Wpointer-arith -Wno-sign-compare ' \
            '-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes',
        CXXWARN='-Wall -Wextra -Wpointer-arith -Wno-sign-compare',
        LDFLAGS='',
        LIBS='',
    )
    osenv = Environment(**kw)
    userenv = defaults + osenv

    # The base environment consist of flags specific to the project.
    baseenv = Environment(
        CPPFLAGS=['-I' + p for p in obj.incldirs],
    )
    return baseenv, userenv

def build_macosx(obj):
    build = target.Build()

    # Get the base environment
    ldflags = ['-Wl,-dead_strip', '-Wl,-dead_strip_dylibs']
    fworks = ['Foundation', 'AppKit', 'OpenGL',
              'CoreServices', 'CoreVideo', 'Carbon']
    for fwork in fworks:
        ldflags.extend(('-framework', fwork))
    baseenv, userenv = env_nix(
        obj,
        ARCHS='ppc i386',
        CC=('gcc-4.2', 'gcc'),
        CXX=('g++-4.2', 'g++'),
        LDFLAGS=ldflags,
    )

    # Build the executable for each architecture
    exes = []
    env = Environment(baseenv, userenv)
    srcs = obj.get_atoms(None, 'MACOSX')
    for arch in env.ARCHS:
        archdir = os.path.join('build', 'arch-' + arch)
        objdir = os.path.join(archdir, 'obj')
        exedir = os.path.join(archdir, 'exe')
        # Build the sources
        archenv = Environment(env, ARCH=arch)
        objs = []
        for src in srcs:
            sbase, sext = os.path.splitext(src)
            stype = path.EXTS[sext]
            if stype in ('c', 'cxx', 'm', 'mm'):
                objf = os.path.join(objdir, sbase + '.o')
                build.add(cc(objf, src, archenv, stype))
                objs.append(objf)
            elif stype in ('h', 'hxx'):
                pass
            else:
                raise Exception('Unknown file type: %r' % src)
        # Build the executable
        exe = os.path.join(exedir, 'Game')
        build.add(ld(exe, objs, archenv))
        exes.append(exe)

    # Combine into Universal binary
    exe = os.path.join('build', 'product', 'Game')
    build.add(lipo(exe, exes, env))
    return build

def build_nix(obj):
    build = target.Build()

    # Get the base environment
    baseenv, userenv = env_nix(
        obj,
        LDFLAGS='-Wl,--as-needed -Wl,--gc-sections',
        LIBS='-lGL -lGLU',
    )
    machine = getmachine(baseenv + userenv)

    # Get the environment for each package
    pkgenvs = {
        None: Environment(),
        'LIBJPEG': Environment(LIBS='-ljpeg'),
        'SDL': customconfig('sdl-config'),
    }
    PKGSPECS = [
        ('SDL', 'sdl >= 1.2.10 sdl < 1.3'),
        ('GTK', 'gtk+-2.0 gtkglext-1.0'),
        ('LIBPNG', 'libpng'),
        ('PANGO', 'pangocairo'),
    ]
    for pkg, spec in PKGSPECS:
        if pkg not in pkgenvs:
            pkgenvs[pkg] = pkgconfig(spec)

    # Build the sources that use each package
    pkgobjs = {}
    for pkg, pkgenv in pkgenvs.iteritems():
        if pkg is None:
            atoms = (None, 'LINUX')
        else:
            atoms = (pkg,)
        objs = []
        pkgobjs[pkg] = objs
        env = Environment(baseenv, pkgenv, userenv)
        for src in obj.get_atoms(*atoms):
            sbase, sext = os.path.splitext(src)
            stype = path.EXTS[sext]
            if stype in ('c', 'cxx'):
                objf = os.path.join('build', 'obj', sbase + '.o')
                build.add(cc(objf, src, env, stype))
                objs.append(objf)
            elif stype in ('h', 'hxx'):
                pass
            else:
                raise Exception('Unknown file type: %r' % src)

    # Build the executables
    COMMON = [None, 'LIBJPEG', 'LIBPNG', 'PANGO']
    EXESPECS = [('gtk', COMMON + ['GTK']),
                ('sdl', COMMON + ['SDL'])]
    exes = []
    for exe, exepkgs in EXESPECS:
        env = Environment(
            *([baseenv] + [pkgenvs[pkg] for pkg in exepkgs] + [userenv]))
        exename = 'game-%s-%s' % (machine, exe)
        exe = os.path.join('build', 'product', exename)
        objs = []
        for pkg in exepkgs:
            objs.extend(pkgobjs[pkg])
        build.add(ld(exe, objs, env))

    return build
