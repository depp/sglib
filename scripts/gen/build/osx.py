from gen.build.nix import cc_cmd, ld_cmd
from gen.build.gmake import Makefile
from gen.env.nix import NixConfig, default_env
from gen.env.env import BuildEnv
from gen.path import Path, TYPE_DESCS
import re

NON_ALPHA_NUM = re.compile('[^A-Za-z0-9]+')
def make_exe_name(name):
    name = NON_ALPHA_NUM.sub('', name)
    if not name:
        name = 'App'
    return name

def gen_makefile(config):
    bcfg = config.get_config('OSX')
    base = default_env(config, 'OSX')
    benv = BuildEnv(
        config.project,
        default_env(config, 'OSX'),
        NixConfig(base))

    makefile = Makefile()
    makefile.gen_regen(bcfg)

    types_cc = 'c', 'cxx', 'm', 'mm'
    types_ignore = 'h', 'hxx'
    all_objects = set()
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
                if obj.posix not in all_objects:
                    all_objects.add(obj.posix)
                    e = benv.env(src.tags)
                    assert e is not None
                    makefile.add_rule(
                        obj, [p],
                        [cc_cmd(e, obj, p, t, depfile=dep)],
                        qname=TYPE_DESCS[t])
                    makefile.opt_include(dep)
            else:
                raise Exception('unknown source type: {}'.format(s.path))
        exe_env = benv.env(target.tag_modules)
        assert exe_env is not None
        app_name = target.target.exe_name['OSX']
        exe_name = make_exe_name(app_name)
        if target.variant.varname != 'cocoa':
            app_name = '{} ({})'.format(app_name, target.variant.varname)
            exe_name = exe_name + target.variant.varname.title()
        obj_exe = Path('build/obj', exe_name)
        makefile.add_rule(
            obj_exe, objects,
            [ld_cmd(exe_env, obj_exe, objects, sourcetypes)],
            qname='Link')
        makefile.add_default(obj_exe)

    with open('Makefile', 'w') as fp:
        makefile.write(fp)
