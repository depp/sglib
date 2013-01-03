import gen.xml as xml
from gen.path import Path
import gen.env.env as env
import gen.target
from gen.error import ConfigError
import os

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

class ProjectConfig(object):
    def __init__(self, project):
        pass

def parse_yesno(p, name, dest=None, default=None, help=None,
                help_neg=None):
    assert dest is not None
    if help_neg is None:
        help_neg = argparse.SUPPRESS
    p.add_argument(
        '--' + name,
        default=default, action='store_true',
        dest=dest, help=help)
    p.add_argument(
        '--no-' + name,
        default=default, action='store_false',
        dest=dest, help=help_neg)

def configure(argv):
    ####################
    # Read project

    proj = load_project(Path('project.xml'))
    cfg = proj.super_config()

    ####################
    # Create parser

    import argparse
    p = argparse.ArgumentParser(
        description='configure the project')

    p.add_argument(
        '--debug', dest='config', default='release',
        action='store_const', const='debug',
        help='use debug configuration')
    p.add_argument(
        '--release', dest='config', default='release',
        action='store_const', const='release',
        help='use release configuration')

    parse_yesno(
        p, 'warnings', dest='warnings',
        help='enable extra compiler warnings',
        help_neg='disable extra compiler warnings')
    parse_yesno(
        p, 'werror', dest='werror',
        help='treat warnings as errors',
        help_neg='do not treat warnings as errors')

    cfg.add_options(p)

    p.add_argument('arg', nargs='*', help=argparse.SUPPRESS)

    ####################
    # Parse options

    opts = p.parse_args(argv)

    errors = []
    targets = []
    envlist = []
    for arg in opts.arg:
        i = arg.find('=')
        if i >= 0:
            name = arg[:i]
            value = arg[i+1:]
            parse = None
            try:
                vartype = env.VAR[name]
            except KeyError:
                pass
            else:
                try:
                    parse = vartype.parse
                except AttributeError:
                    pass
            if parse is None:
                errors.append(
                    ConfigError('unknown environment variable: {}'
                                .format(name)))
                continue
            value = parse(value)
            envlist.append([{ name: value }])
        else:
            targets.append(arg)
    base_env = env.merge_env(envlist)
    if not targets:
        targets = ['default']

    if len(targets) > 1:
        errors.append(
            ConfigError('too many targets: {}'.format(', '.join(targets))))

    if errors:
        raise ConfigError(
            'error parsing arguments',
            suberrors=errors)

    target_func = gen.target.get_target(targets[0])
    return target_func(project, opts)
