import gen.build.graph
from gen.env import Environment
import optparse
import sys

def run(proj):
    """Build the given project using the command line options.

    The arguments are parsed for options, environment variables, and
    build targets.  The options and environmment variables are stored
    in a new Build object, and each target is invoked.  If no targets
    are specified, the default target for the current platform is
    invoked.
    """
    p = optparse.OptionParser()
    p.add_option('--dump-env', dest='dump_env',
                 action='store_true', default=False,
                 help='print all environment variables')
    opts, args = p.parse_args()

    targets = []
    env = Environment()
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

    graph = gen.build.graph.Graph()
    for module in ('linux',): # osx msvc
        __import__('gen.build.' + module)
        m = getattr(gen.build, module)
        m.add_targets(graph, proj, env)

    r = graph.build(targets)
    if r:
        sys.exit(0)
    else:
        sys.exit(1)
