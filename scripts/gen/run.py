#import gen.config as config
from gen.error import ConfigError
from gen.path import Path
import gen.xml as xml
import os
import sys

#CACHE_FILE = config.CACHE_FILE
CACHE_FILE = 'config.dat'

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

DEFAULT_ACTIONS = {
    'linux': ('makefile', 'runner', 'version', 'header'),
    'osx': ('makefile', 'version', 'header'),
    'windows': ('version', 'header'),
}

def load_project(path):
    """Load a project and all required modules.

    This will also scan for bundled libraries in the directory
    specified by the project file.
    """
    errors = []

    xmlfiles = [path]
    with open(path.native, 'rb') as fp:
        proj = xml.load(fp, path.dirname)

    bundled_libs = []
    for lib_path in proj.lib_path:
        for fname in os.listdir(lib_path.native):
            if fname.startswith('.'):
                continue
            try:
                path = Path(fname)
            except ValueError:
                continue
            path = Path(lib_path, path)
            if os.path.isdir(path.native):
                bundled_libs.append(path)

    mod_paths = list(proj.module_path)
    mod_deps = set()
    for target in proj.targets:
        mod_deps.update(target.module_deps())
    q = list(mod_deps)
    while q:
        modid = q.pop()
        try:
            module = proj.modules[modid]
        except KeyError:
            for mod_path in mod_paths:
                try:
                    fp = open(Path(mod_path, modid + '.xml').native, 'rb')
                except OSError:
                    continue
                with fp:
                    module = xml.load(fp, mod_path)
                    proj.modules[modid] = module
                break
            else:
                errors.append(
                    ConfigError('cannot find module: {}'.format(modid)))
                continue
        module.scan_bundled_libs(bundled_libs)
        new_mod_deps = set(module.module_deps()).difference(mod_deps)
        mod_deps.update(new_mod_deps)
        q.extend(new_mod_deps)

    proj.modules = { k: v for k, v in proj.modules.items()
                     if k in mod_deps }

    try:
        proj.validate()
    except ConfigError as ex:
        errors.append(ex)

    if errors:
        raise ConfigError('error in project', suberrors=errors)

    return proj

def run():
    try:
        proj = load_project(Path('project.xml'))
        return
        mode = sys.argv[1]
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
        ex.write(sys.stderr)
        sys.exit(1)
