import gen.config as config
from gen.error import ConfigError
#from gen.path import Path
#import gen.xml as xml
import os
import sys
import pickle

CACHE_FILE = 'config.dat'

def load():
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
    try:
        os.unlink(CACHE_FILE)
    except OSError:
        pass
    try:
        with open(CACHE_FILE + '.tmp', 'wb') as fp:
            pickle.dump(cfg, fp, pickle.HIGHEST_PROTOCOL)
        os.rename(CACHE_FILE + '.tmp', CACHE_FILE)
    except:
        try:
            os.unlink(CACHE_FILE + '.tmp')
        except OSError:
            pass

def run():
    try:
        mode = sys.argv[1]
        quiet = False
        if mode == 'config':
            argv = sys.argv[2:]
            target = config.configure(argv)
            store((argv, pickle.dumps(target, pickle.HIGHEST_PROTOCOL)))
            for f in target.files:
                target.build(f)
        elif mode == 'reconfig':
            argv, target = load()
            target = config.configure(argv)
            store((argv, pickle.dumps(target, pickle.HIGHEST_PROTOCOL)))
            for f in target.files:
                target.build(f)
        elif mode == 'build':
            argv, target = load()
            target = pickle.loads(target)
            target.build(sys.argv[2])
        else:
            sys.stderr.write('error: invalid mode: {}\n'.format(mode))
            sys.exit(1)
    except ConfigError as ex:
        ex.write(sys.stderr)
        sys.exit(1)
