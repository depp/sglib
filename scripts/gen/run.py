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
    try:
        quiet = False
        if mode == 'config':
            argv = sys.argv[2:]
            cfg, actions = config.configure(argv)
            store((argv, cfg))
        elif mode == 'reconfig':
            argv, cfg = load()
            cfg = config.reconfigure(argv)
            store((argv, cfg))
            actions = []
        elif mode == 'build':
            argv, cfg = load()
            actions = sys.argv[2:]
            quiet = True
        else:
            sys.stderr.write('error: invalid mode: {}\n'.format(mode))
            sys.exit(1)
        for action in actions:
            cfg.exec_action(action, quiet)
    except ConfigError as ex:
        sys.stderr.write('\n')
        sys.stderr.write('error: ' + ex.reason + '\n')
        if ex.details is not None:
            sys.stderr.write(ex.details)
        sys.exit(1)
