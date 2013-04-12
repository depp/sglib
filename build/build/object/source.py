from build.error import ConfigError
import collections

Source = collections.namedtuple(
    'Source', 'path type header_paths modules defs external')

class SourceModule(object):
    __slots__ = ['sources', 'public_modules', 'private_modules',
                 'header_paths', 'defs']

    def __init__(self):
        self.sources = []
        self.public_modules = set()
        self.private_modules = set()
        self.header_paths = set()
        self.defs = {}

    @classmethod
    def parse(class_, mod, *, external=False):
        smod = class_()
        def add_group(group, ipaths, reqs, defs):
            ipaths = ipaths.union(p.path for p in group.header_paths)
            smod.header_paths.update(p.path for p in group.header_paths
                                     if p.public)
            reqs = reqs.union(m.module for m in group.requirements)
            smod.private_modules.update(m.module for m in group.requirements)
            smod.public_modules.update(m.module for m in group.requirements
                                       if m.public)
            if group.defs:
                defs = dict(defs)
                for def_ in group.defs:
                    if def_.public:
                        if (def_.name in smod.defs and
                            smod.defs[def_.name] != def_.value):
                            raise ConfigError(
                                'conflicting preprocessor definiton: {!r}'
                                .format(def_.name))
                        smod.defs[def_.name] = def_.value
                    defs[def_.name] = def_.value
            smod.sources.extend(
                Source(src.path, src.type, ipaths, reqs, defs, external)
                for src in group.sources)
            for subgroup in group.groups:
                add_group(subgroup, ipaths, reqs, defs)
        add_group(mod.group, frozenset(), frozenset(), {})
        return smod

    def dump(self):
        print('Source module')
        print('  Public header paths:')
        for ipath in self.header_paths:
            print('    {}'.format(ipath))
        print('  Public module refs:')
        for mod in self.public_modules:
            print('    {}'.format(mod))
        print('  Private module refs:')
        for mod in self.private_modules:
            print('    {}'.format(mod))
        print('  Sources:')
        for src in self.sources:
            print('    {}'.format(src.path))

class FlatSourceModule(object):
    """A flat source module has dependencies only on non-source modules."""
    __slots__ = ['sources', 'deps', 'header_paths', 'defs']

    def __init__(self, sources, deps):
        self.sources = sources
        self.deps = deps
        ipaths = set()
        defs = {}
        for src in self.sources:
            ipaths.update(src.header_paths)
            for k, v in src.defs.items():
                if k in defs and defs[k] != v:
                    raise ConfigError(
                        'conflicting preprocessor definiton: {!r}'
                        .format(def_.name))
                defs[k] = v
        self.header_paths = tuple(sorted(ipaths))
        self.defs = defs

def resolve_sources(modules):
    """Resolve all dependencies among source modules.

    This generates flat source modules and a list of sources.
    """
    dep_private = {}
    dep_public = {}
    for modname, mod in modules.items():
        modules[modname] = mod
        dep_public[modname] = set(mod.public_modules)
        dep_private[modname] = set(mod.private_modules)
        dep_public[modname].add(modname)
        dep_private[modname].add(modname)
    for k in modules:
        for i in modules:
            for j in modules:
                if k in dep_private[i] and j in dep_private[k]:
                    dep_private[i].add(j)
                if k in dep_public[i] and j in dep_public[k]:
                    dep_public[i].add(j)
    intern_srcmods = {}
    intern_ipaths = {}
    intern_defs = {}
    sources = {}
    msources = {}
    for mname in modules:
        msrc = {}
        for src in modules[mname].sources:
            srcmods = set(src.modules)
            for srcmod in src.modules:
                try:
                    srcmods.update(dep_public[srcmod])
                except KeyError:
                    pass
            ipaths = set(src.header_paths)
            defs = dict(src.defs)
            for srcmod in srcmods:
                try:
                    m = modules[srcmod]
                except KeyError:
                    pass
                else:
                    ipaths.update(m.header_paths)
                    for k, v in m.defs.items():
                        if k in defs and defs[k] != v:
                            raise ConfigError(
                                'conflicting preprocessor definiton: {!r}'
                                .format(k))
                        defs[k] = v
            srcmods.difference_update(modules)
            ipaths = tuple(sorted(ipaths))
            ipaths = intern_ipaths.setdefault(ipaths, ipaths)
            srcmods = tuple(sorted(srcmods))
            srcmods = intern_srcmods.setdefault(srcmods, srcmods)
            defs = intern_defs.setdefault(tuple(sorted(defs.items())), defs)
            src2 = Source(src.path, src.type, ipaths, srcmods,
                          defs, src.external)
            try:
                src3 = sources[src.path]
            except KeyError:
                sources[src.path] = src2
            else:
                if src2 != src3:
                    raise ConfigError(
                        'conflicting entries for source file: {}'
                        .format(src2.path))
                src2 = src3
            msrc[src2.path] = src2
        msources[mname] = msrc
    nmods = {}
    for mname in modules:
        mdeps = set()
        for dep in dep_private[mname].intersection(modules):
            mdeps.update(dep_private[dep])
        msrc = {}
        for dep in mdeps.intersection(modules):
            msrc.update(msources[dep])
        msrc = list(msrc.values())
        msrc.sort()
        mdeps.difference_update(modules)
        mdeps = list(mdeps)
        mdeps.sort()
        nmod = FlatSourceModule(msrc, mdeps)
        nmods[mname] = nmod
    return nmods, sources
