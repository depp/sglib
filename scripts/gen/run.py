import gen.config as config
from gen.error import ConfigError
import os
import sys

CACHE_FILE = config.CACHE_FILE

def load():
    import pickle
    try:
        fp = open(CACHE_FILE, 'rb')
        obj = pickle.load(fp)
        fp.close()
        return obj
    except (OSError, pickle.PickleError):
        sys.stderr.write(
            'error: could not load configuration cache\n'
            'please run init.sh again\n')
        sys.exit(1)

def store(cfg):
    import pickle
    try:
        os.unlink(CACHE_FILE)
    except OSError:
        pass
    try:
        with open(CACHE_FILE, 'wb') as fp:
            pickle.dump(cfg, fp, pickle.HIGHEST_PROTOCOL)
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
            actions = cfg.reconfig()
            store(cfg)
        elif mode == 'reconfig':
            cfg = load()
            cfg.quiet = True
            cfg.reconfig()
            store(cfg)
            actions = []
        elif mode == 'build':
            cfg = load()
            cfg.quiet = True
            actions = argv
        else:
            sys.stderr.write('error: invalid mode: {}\n'.format(mode))
            sys.exit(1)
        for action in actions:
            cfg.exec_action(action)
    except ConfigError as ex:
        sys.stderr.write('\n')
        sys.stderr.write('error: ' + ex.reason + '\n')
        if ex.details is not None:
            sys.stderr.write(ex.details)
        sys.exit(1)
