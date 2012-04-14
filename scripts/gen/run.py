import gen.build.graph as graph
import gen.build.target as target
import gen.smartdict as smartdict
from gen.env import Environment, BuildSettings
from gen.path import Path
import gen.git as git
import optparse
import sys

MODULES = ['version', 'linux', 'osx']
#MODULES = ['version', 'linux', 'osx', 'msvc', 'gmake']
# archive must be last
# MODULES.append('archive')

def run(proj):
    """Build the given project using the command line options.

    The arguments are parsed for options, environment variables, and
    build targets.  The options and environmment variables are stored
    in a new Build object, and each target is invoked.  If no targets
    are specified, the default target for the current platform is
    invoked.
    """

    # Check for duplicate source files
    src = set()
    for s in proj.sourcelist.sources():
        p = s.relpath.posix
        if p in src:
            raise Exception('duplicate source file: %s' % (p,))
        src.add(p)
    del src

    # Parse options
    p = optparse.OptionParser()
    p.add_option('--dump-env', dest='dump_env',
                 action='store_true', default=False,
                 help='print all environment variables')
    opts, args = p.parse_args()
    targets = []
    env = Environment(GIT='git')
    settings = BuildSettings()
    envs = [env, settings, proj.info]
    for arg in args:
        idx = arg.find('=')
        if idx >= 0:
            var = arg[:idx]
            val = arg[idx+1:]
            for e in envs:
                try:
                    e[var] = val
                except smartdict.UnknownKey:
                    pass
                else:
                    break
            else:
                print >>sys.stderr, \
                    'error: unknown variable: %r' % (var,)
                sys.exit(1)
        else:
            targets.append(arg)
    if not targets:
        targets = ['default']

    # Get the version information
    vers = [('PKG_SG_VERSION',  lambda: git.get_version(env, proj.sgpath)),
            ('PKG_SG_COMMIT',   lambda: git.get_sha1(env, proj.sgpath)),
            ('PKG_APP_VERSION', lambda: git.get_version(env, Path('.'))),
            ('PKG_APP_COMMIT',  lambda: git.get_sha1(env, Path('.')))]
    for key, val in vers:
        if key not in proj.info:
            proj.info[key] = val()

    # Generate build rules
    ms = []
    for module in MODULES:
        m = __import__('gen.build.' + module)
        m = getattr(m.build, module)
        ms.append(m)
    g = graph.Graph()
    for m in ms:
        m.add_sources(g, proj, env, settings)
    g.add(target.DepTarget('built-sources', g.targets()))
    for m in ms:
        m.add_targets(g, proj, env, settings)

    # Run the build
    r = g.build(targets, settings)
    if r:
        sys.exit(0)
    else:
        sys.exit(1)
