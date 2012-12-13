from gen.build.nix import cc_cmd, ld_cmd
from gen.build.gmake import Makefile
from gen.env.nix import NixConfig, default_env, getmachine
from gen.env.env import BuildEnv
from gen.path import Path, TYPE_DESCS
import sys

def extract_debug(dest, src):
    """Return commands for extracting debug info from an object."""
    return [['objcopy', '--only-keep-debug', src.posix, dest.posix],
            ['chmod', '-x', dest.posix]]

def strip(dest, src, debugsyms):
    """Return commands to strip an object.

    A debug link is added to the debugsyms object if it is not None.
    """
    cmd = ['objcopy', '--strip-unneeded']
    if debugsyms is not None:
        cmd.append('--add-gnu-debuglink=' + debugsyms.posix)
    cmd.extend((src.posix, dest.posix))
    return [cmd]

def gen_regen(makefile, bcfg):
    from gen.config import CACHE_FILE, DEFAULT_ACTIONS
    import os.path
    spath = os.path.abspath(__file__)
    for i in range(3):
        spath = os.path.dirname(spath)
    spath = os.path.relpath(os.path.join(spath, 'init.py'))
    config_script = [sys.executable, spath]
    cache_file = Path(CACHE_FILE)
    makefile.add_rule(
        cache_file, bcfg.projectconfig.xmlfiles,
        [[sys.executable, spath, 'reconfig']],
        qname='Reconfigure')
    actions = bcfg.projectconfig.actions
    for action_name in DEFAULT_ACTIONS[bcfg.os]:
        target = actions[action_name][0]
        makefile.add_rule(
            target, [cache_file],
            [[sys.executable, spath, 'build', action_name]],
            qname='Regen')
        if target.posix != 'Makefile':
            makefile.add_default(target)

def gen_makefile(config):
    bcfg = config.get_config('LINUX')
    base = default_env(config, 'LINUX')
    benv = BuildEnv(
        config.project,
        default_env(config, 'LINUX'),
        NixConfig(base))

    machine = getmachine(base)
    makefile = Makefile()
    gen_regen(makefile, bcfg)

    types_cc = 'c', 'cxx'
    types_ignore = 'h', 'hxx'
    sources = set()
    for target in bcfg.targets:
        objects = []
        sourcetypes = set()
        for src in target.sources():
            p = src.path
            t = src.sourcetype
            sourcetypes.add(t)
            if t in types_ignore:
                pass
            elif t in types_cc:
                obj = Path('build/obj', p.withext('.o'))
                dep = obj.withext('.d')
                objects.append(obj)
                if p.posix not in sources:
                    sources.add(p.posix)
                    e = benv.env(src.tags)
                    assert e is not None
                    makefile.add_rule(obj, [p],
                                      [cc_cmd(e, obj, p, t, depfile=dep)],
                                      qname=TYPE_DESCS[t])
                    makefile.opt_include(dep)
            else:
                raise Exception('unknown source type: {}'.format(s.path))
        exe_env = benv.env(target.tag_modules)
        assert exe_env is not None
        exe_name = '{}_{}_{}'.format(
            target.target.exe_name['LINUX'],
            machine,
            target.variant.varname.lower())
        obj_exe = Path('build/obj', exe_name)
        prod_exe = Path('build/product', exe_name)
        prod_dbg = prod_exe.addext('.dbg')
        makefile.add_rule(
            obj_exe, objects,
            [ld_cmd(exe_env, obj_exe, objects, sourcetypes)],
            qname='Link')
        makefile.add_rule(
            prod_dbg, [obj_exe],
            extract_debug(prod_dbg, obj_exe),
            qname='ObjCopy')
        makefile.add_rule(
            prod_exe, [obj_exe, prod_dbg],
            strip(prod_exe, obj_exe, prod_dbg),
            qname='Strip')
        makefile.add_default(prod_exe)

    with open('Makefile', 'w') as fp:
        makefile.write(fp)
