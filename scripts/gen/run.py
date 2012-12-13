import gen.config as config
from gen.error import ConfigError
import os
import sys

CACHE_FILE = 'init.cache.dat'

def load():
    import cPickle
    try:
        fp = open(CACHE_FILE, 'rb')
        obj = cPickle.load(fp)
        fp.close()
        return obj
    except (OSError, cPickle.PickleError):
        sys.stderr.write(
            'error: could not load configuration cache\n'
            'please run init.sh again\n')
        sys.exit(1)

def store(cfg):
    import cPickle
    try:
        os.unlink(CACHE_FILE)
    except OSError:
        pass
    try:
        with open(CACHE_FILE, 'wb') as fp:
            cPickle.dump(cfg, fp, cPickle.HIGHEST_PROTOCOL)
    except:
        try:
            os.unlink(CACHE_FILE)
        except OSError:
            pass

def run():
    mode = sys.argv[1]
    argv = sys.argv[2:]
    try:
        if mode == 'config':
            cfg = config.ProjectConfig()
            cfg.argv = argv
            targets = cfg.reconfig()
            store(cfg)

            import gen.env.nix as nix
            import gen.env.env as env
            build_cfg = cfg.get_config('LINUX')
            base_env = nix.default_env(cfg, 'LINUX')
            print base_env
            build_env = env.BuildEnv(
                cfg.project, base_env, nix.NixConfig(base_env))

            build_cfg.dump()

            for t in build_cfg.targets:
                print '-----'
                t.dump()
                for s in t.sources():
                    e = build_env.env(s.tags)
                    if e is None:
                        print 'SKIP:', s.path.posix
                    else:
                        print s.path.posix, ' '.join(s.tags)
        elif mode == 'reconfig':
            cfg = load()
            cfg.reconfig()
            store(cfg)
            targets = []
        elif mode == 'build':
            cfg = load()
            targets = argv
        else:
            sys.stderr.write('error: invalid mode: %s\n' % mode)
            sys.exit(1)
    except ConfigError, ex:
        sys.stderr.write('\n')
        sys.stderr.write('error: ' + ex.reason + '\n')
        if ex.details is not None:
            sys.stderr.write(ex.details)
        sys.exit(1)
