import buildtool.path as path
import buildtool.shell as shell
import buildtool.build.target as target
from buildtool.env import Environment
import os
import shutil

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
    return target.Command(cmd, inputs=[src], outputs=[obj],
                          quietmsg=buildline(what, src, tag))

def ld(obj, src, env):
    """Create a target that links an executable."""
    cc = env.CXX
    # Some LDFLAGS do not work if they don't appear before -o
    tag, aflags = getarch(env)
    cmd = [cc] + aflags + env.LDFLAGS + ['-o', obj] + src + env.LIBS
    return target.Command(cmd, inputs=src, outputs=[obj],
                          quietmsg=buildline('LD', obj, tag))

def lipo(obj, src, env):
    """Mac OS X: Create a universal executable from non-universal ones."""
    cmd = ['lipo'] + src + ['-create', '-output', obj]
    return target.Command(cmd, inputs=src, outputs=[obj],
                          quietmsg=buildline('LIPO', obj, None))

def ibtool(obj, src, env):
    ibtool = '/Developer/usr/bin/ibtool'
    cmd = [ibtool, '--errors', '--warnings', '--notices',
           '--output-format', 'human-readable-text',
           '--compile', obj, src]
    return target.Command(cmd, inputs=[src], outputs=[obj],
                          quietmsg=buildline('IBTOOL', obj, None))

def plist(obj, src, changes):
    # PlistBuddy operates in-place, so we use a pre-command hook
    # to copy our source to the destination
    def pre():
        print 'PRE hook'
        shutil.copyfile(src, obj)
        return True
    buddy = '/usr/libexec/PlistBuddy'
    cmd = [buddy, '-x']
    for change in changes:
        cmd.extend(('-c', change))
    cmd.append(obj)
    return target.Command(cmd, inputs=[src], outputs=[obj],
                          quietmsg=buildline('PLIST', obj, None),
                          pre=pre)

def env_nix(obj, **kw):
    # Base environment common to Mac OS X / Linux / etc.  This always
    # comes first, and the user environment always comes last.  This
    # way, flags in the user environment can override flags in the
    # base environment.
    baseenv = Environment(
        CPPFLAGS=['-I' + p for p in obj.incldirs],
    )

    # Common defaults.  Some of these are overridden by platform
    # variations.  All may be overridden by the user.
    userenv = Environment(
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
    userenv.override(Environment(**kw))
    userenv.override(obj.env)

    return baseenv, userenv

def build_macosx(obj):
    build = target.Build()

    # Get the environments
    baseenv, userenv = env_nix(
        obj,
        ARCHS='ppc i386',
        CC=('gcc-4.2', 'gcc'),
        CXX=('g++-4.2', 'g++'),
        LDFLAGS='-Wl,-dead_strip -Wl,-dead_strip_dylibs',
    )
    ldflags = []
    fworks = ['Foundation', 'AppKit', 'OpenGL',
              'CoreServices', 'CoreVideo', 'Carbon']
    for fwork in fworks:
        ldflags.extend(('-framework', fwork))
    baseenv = Environment(baseenv, LDFLAGS=ldflags)
    env = Environment(baseenv, userenv)

    appname = userenv.EXE_MAC
    exename = appname

    # Build the executable for each architecture
    exes = []
    srcs = obj.get_atoms(None, 'MACOSX')
    for arch in env.ARCHS:
        archdir = os.path.join('build', 'arch-' + arch)
        objdir = os.path.join(archdir, 'obj')
        exedir = os.path.join(archdir, 'exe')
        # Build the sources
        if arch in ('ppc', 'ppc64'):
            cflags = '-mtune=G5'
        else:
            cflags = ''
        archenv = Environment(ARCH=arch, CFLAGS=cflags, CXXFLAGS=cflags)
        archenv = Environment(baseenv, archenv, userenv)
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
        exe = os.path.join(exedir, exename)
        build.add(ld(exe, objs, archenv))
        exes.append(exe)

    contents = os.path.join('build', 'product', appname + '.app', 'Contents')

    # Combine into Universal binary
    exe = os.path.join(contents, 'MacOS', exename)
    build.add(lipo(exe, exes, env))

    # Create Info.plist and PkgInfo
    pcmds = ['Set :CFBundleExecutable %s' % exename]
    # CFBundleIdentifier
    build.add(plist(os.path.join(contents, 'Info.plist'),
                    'mac/Game-Info.plist', pcmds))
    pkginfo = 'APPL????'
    build.add(target.StaticFile(os.path.join(contents, 'PkgInfo'),
                                pkginfo))

    # Compile / copy resources
    resources = os.path.join(contents, 'Resources')
    build.add(ibtool(os.path.join(resources, 'MainMenu.nib'),
                     'mac/MainMenu.xib', env))

    return build

def build_nix(obj):
    build = target.Build()

    # Get the base environment
    baseenv, userenv = env_nix(
        obj,
        LDFLAGS='-Wl,--as-needed -Wl,--gc-sections',
    )

    machine = getmachine(Environment(baseenv, userenv))

    # Get the environment for each package
    pkgenvs = {
        None: Environment(LIBS='-lGL -lGLU'),
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
        exename = '%s-%s-%s' % (env.EXE_LINUX, machine, exe)
        exe = os.path.join('build', 'product', exename)
        objs = []
        for pkg in exepkgs:
            objs.extend(pkgobjs[pkg])
        build.add(ld(exe, objs, env))

    return build
