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

class SimpleComponent(object):
    __slots__ = ['obj']

    def __init__(self, obj):
        self.obj = obj

class ExtLibComponent(object):
    __slots__ = ['obj']

    def __init__(self, obj):
        self.obj = obj

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
        self.enable, self.variants = (project.super_config()
                                      .get_enables(opts, os))
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
        priv_mods, cvars = bconfig.config.variant_config(flagids)
        result = vconfig(bconfig, priv_mods, cvars)
        self._vconfig[vkey] = result
        return result

    def configure(self):
        for modid in self.project.modules:
            try:
                closure = self.get_closure(modid)
                bconfig = self.get_bconfig('module:' + modid)
            except ConfigError as ex:
                print('{}: <error>'.format(modid))
            else:
                print('{}: ({}) {{{}}}'
                      .format(modid, ', '.join(sorted(closure)),
                              ','.join('{}={}'.format(k, v)
                                       for k, v in bconfig.pub_env.items())))
