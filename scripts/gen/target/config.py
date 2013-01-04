import gen.target as target
import gen.project.module as module
import gen.env as env
from gen.error import ConfigError
import collections

# Base config
bconfig = collections.namedtuple(
    'bconfig', 'config srcmodule pub_mods global_mods pub_env priv_env')
vconfig = collections.namedtuple(
    'vconfig', 'bconfig priv_mods cvars')

def check_sources(x, y):
    if x.envs != y.envs:
        raise ConfigError(
            'duplicate source files cannot be merged',
            '"{}" has multiple entries with different build environments'
            .format(src.path.posix))
    if x.generator != y.generator:
        raise ConfigError(
            'duplicate source files cannot be merged',
            '"{}" has multiple entries with different generators'
            .format(src.path.posix))

class TargetConfig(object):
    """This is a base class for target configs.

    The 'config_pkgconfig', 'config_sdlconfig', 'config_framework',
    and 'config_librarysearch' methods should be overridden.
    """
    __slots__ = [
        'opts', 'base_env', 'os', 'project', 'enable', 'variants',
        '_components', '_bconfig', '_vconfig', '_closure'
    ]

    def __init__(self, opts, base_env, os, project):
        self.opts = opts
        self.base_env = base_env
        self.os = os
        self.project = project
        cfg = project.super_config()
        self.enable, variants = cfg.get_enables(opts, os)
        variant_table = cfg.variants()
        self.variants = { k: v for k, v in variant_table.items()
                          if k in variants }
        print('enable: {}'.format(', '.join(self.enable)))
        print('variants: {}'.format(', '.join(self.variants)))
        self._components = {}
        for target in project.targets:
            self._components['target:' + target.filename[os]] = target
        for modid, obj in project.modules.items():
            self._components['module:' + modid] = obj
        self._bconfig = {}
        self._vconfig = {}
        self._closure = {}

    def get_bconfig_simple(self, obj, srcmodule):
        """Get the base config for a simple component."""
        config = srcmodule.config
        pub_mods, global_mods, pub_env, priv_env = config.apply_config(self)
        return bconfig(config, srcmodule,
                       pub_mods, global_mods, pub_env, priv_env)

    def get_bconfig_extlib(self, obj):
        """Get the base config for an external library component."""
        errors = []
        result = None
        for config in obj.libsources:
            try:
                result = config.apply_config(self)
            except ConfigError as ex:
                errors.append(ex)
            else:
                break
        else:
            raise ConfigError(
                'could not configure {}'.format(obj.name),
                suberrors=errors)
        pub_mods, global_mods, pub_env, priv_env = result
        pub_env = env.merge_env([pub_env, priv_env])
        priv_env = {}
        return bconfig(config, None, pub_mods, None, pub_env, priv_env)

    def get_bconfig(self, key):
        """Get the base config for the given component."""
        try:
            return self._bconfig[key]
        except KeyError:
            pass
        if key == 'project':
            config = self.project.config
            result = config.apply_config(self)
            pub_mods, global_mods, pub_env, priv_env = result
            pub_env = env.merge_env([pub_env, priv_env])
            result = bconfig(config, None, pub_mods, None, pub_env, {})
        else:
            obj = self._components[key]
            srcmodule = obj.srcmodule
            if isinstance(obj, module.ExternalLibrary):
                assert key.startswith('module:')
                modid = key[key.index(':')+1:]
                if srcmodule is not None:
                    flag = getattr(self.opts, 'bundle:' + modid)
                    if flag is None:
                        flag = True
                    if not flag:
                        srcmodule = None
                if srcmodule:
                    result = self.get_bconfig_simple(obj, srcmodule)
                else:
                    result = self.get_bconfig_extlib(obj)
            else:
                result = self.get_bconfig_simple(obj, srcmodule)
        self._bconfig[key] = result
        return result

    def get_closure(self, modid):
        """Get the closure of a module's public dependencies."""
        try:
            return self._closure[modid]
        except KeyError:
            pass
        cdict = { }
        q = [modid]
        mods = set(q)
        while q:
            qmodid = q.pop()
            bconfig = self.get_bconfig('module:' + qmodid)
            cdict[qmodid] = set(bconfig.pub_mods)
            for cmodid in bconfig.pub_mods:
                try:
                    closure = self._closure[cmodid]
                except KeyError:
                    if cmodid not in mods:
                        mods.add(cmodid)
                        q.append(cmodid)
                else:
                    cdict[qmodid].update(closure)
        while True:
            changed = False
            for qmodid in mods:
                closure = cdict[qmodid]
                nclosure = set(closure)
                for cmodid in closure:
                    try:
                        cclosure = cdict[cmodid]
                    except KeyError:
                        pass
                    else:
                        nclosure.update(cclosure)
                if closure != nclosure:
                    changed = True
                    cdict[qmodid] = nclosure
            if not changed:
                break
        for qmodid, closure in cdict.items():
            self._closure[qmodid] = frozenset(closure)
        return self._closure[modid]

    def get_vconfig(self, key, variant):
        """Get the variant config for a given component."""
        vkey = '{}:{}'.format('*' if variant is None else variant, key)
        try:
            return self._vconfig[key]
        except KeyError:
            pass
        bconfig = self.get_bconfig(key)
        flagids = set(self.enable)
        if variant is not None:
            flagids.add(variant)
        config = bconfig.config
        priv_mods, cvars = config.variant_config(flagids)
        result = vconfig(bconfig, priv_mods, cvars)
        self._vconfig[vkey] = result
        return result

    def configure(self):
        targets = []
        built_sources = {}
        for ptarget in self.project.targets:
            tname = 'target:' + ptarget.filename[self.os]
            for variantid, variant in self.variants.items():
                flagids = set(self.enable)
                flagids.add(variantid)
                sources = {}
                q = ['project', tname]
                components = set([tname])
                envs = []
                cvars = []
                while q:
                    cid = q.pop()
                    vconfig = self.get_vconfig(cid, variantid)
                    cvars.append(vconfig.cvars)
                    bconfig = vconfig.bconfig
                    envs.extend((bconfig.pub_env, bconfig.priv_env))
                    for modid in vconfig.priv_mods:
                        mcid = 'module:' + modid
                        if mcid in components:
                            continue
                        components.add(mcid)
                        q.append(mcid)
                    if bconfig.srcmodule is None:
                        continue
                    gcomps = set('module:' + modid
                                 for modid in bconfig.global_mods)
                    gcomps.add(cid)
                    gcomps.add('project')
                    for tsrc in bconfig.srcmodule.sources:
                        if not flagids.issuperset(tsrc.enable):
                            continue
                        stype = tsrc.sourcetype
                        if stype in ('c', 'cxx', 'm', 'mm'):
                            comps = set(gcomps)
                            comps.update('module:' + modid
                                         for modid in tsrc.module)
                            for ccid in tuple(comps):
                                if ccid.startswith('module:'):
                                    modid = ccid[ccid.index(':')+1:]
                                    comps.update(
                                        'module:' + modid
                                        for modid in self.get_closure(modid))
                            senvs = [bconfig.priv_env]
                            senvs.extend(self.get_bconfig(ccid).pub_env
                                         for ccid in sorted(comps))
                            senvs = tuple([e for e in senvs if e])
                        else:
                            senvs = ()
                        src = target.Source(
                            tsrc.path, senvs, tsrc.generator, stype)
                        if src.generator:
                            try:
                                osrc = built_sources[tsrc.path]
                            except KeyError:
                                built_sources[tsrc.path] = src
                            else:
                                check_sources(src, osrc)
                                src = osrc
                        try:
                            osrc = sources[tsrc.path]
                        except KeyError:
                            pass
                        else:
                            check_sources(src, osrc)
                            continue
                        sources[tsrc.path] = src

                suffix = variant.filename[self.os]
                if suffix:
                    if self.os == 'linux':
                        filename = '{}_{}'.format(tname, suffix)
                    elif self.os == 'windows':
                        filename = '{} {}'.format(tname, suffix)
                    elif self.os == 'osx':
                        filename = '{} ({})'.format(tname, suffix)
                    else:
                        filename = '{}_{}'.format(tname, suffix)
                else:
                    filename = tname
                sources = tuple(v for k, v in sorted(sources.items()))
                ncvars = []
                cvar_names = set()
                for cvarlist in cvars:
                    for name, value in reversed(cvarlist):
                        if name in cvar_names:
                            continue
                        cvar_names.add(name)
                        ncvars.append((name, value))
                ncvars.reverse()
                exe = target.Executable(
                    filename,
                    tuple(ptarget.exe_icon),
                    ptarget.apple_category,
                    tuple(ncvars),
                    tuple(e for e in envs if e),
                    sources)
                targets.append(exe)
        return target.Project(
            tuple(sorted(targets, key=lambda x:x.filename)),
            { k: v.generator for k, v in built_sources.items() })
