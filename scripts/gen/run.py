import gen.build.graph as graph
import gen.build.target as target
from gen.env import Environment
import gen.git as git
import optparse
import sys

MODULES = ['version', 'linux', 'osx', 'msvc', 'gmake']
# archive must be last
MODULES.append('archive')

def run(proj):
    """Build the given project using the command line options.

    The arguments are parsed for options, environment variables, and
    build targets.  The options and environmment variables are stored
    in a new Build object, and each target is invoked.  If no targets
    are specified, the default target for the current platform is
    invoked.
    """
    src = set()
    for s in proj.sourcelist.sources():
        p = s.relpath.posix
        if p in src:
            raise Exception('duplicate source file: %s' % (p,))
        src.add(p)

    p = optparse.OptionParser()
    p.add_option('--dump-env', dest='dump_env',
                 action='store_true', default=False,
                 help='print all environment variables')
    opts, args = p.parse_args()

    targets = []
    env = Environment()
    env.GIT='git'
    for arg in args:
        idx = arg.find('=')
        if idx >= 0:
            var = arg[:idx]
            val = arg[idx+1:]
            env[var] = val
        else:
            targets.append(arg)

    if not targets:
        targets = ['default']

    venv = Environment(
        PKG_SG_VERSION=git.get_version(env, proj.sgpath),
        PKG_SG_COMMIT=git.get_sha1(env, proj.sgpath),
        PKG_APP_VERSION=git.get_version(env, '.'),
        PKG_APP_COMMIT=git.get_sha1(env, '.'),
    )
    env = Environment(venv, env)

    ms = []
    for module in MODULES:
        m = __import__('gen.build.' + module)
        m = getattr(m.build, module)
        ms.append(m)

    g = graph.Graph()
    for m in ms:
        m.add_sources(g, proj, env)
    g.add(target.DepTarget('built-sources', g.targets(), env))
    for m in ms:
        m.add_targets(g, proj, env)

    r = g.build(targets, env)
    if r:
        sys.exit(0)
    else:
        sys.exit(1)
