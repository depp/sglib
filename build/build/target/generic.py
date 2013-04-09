import collections

Source = collections.namedtuple(
    'Source', 'path type header_paths modules')
class SourceModule(object):
    __slots__ = ['name', 'sources', 'public_modules', 'private_modules',
                 'header_paths']
    is_target = False

    def __init__(self, name):
        self.name = name
        self.sources = []
        self.public_modules = set()
        self.private_modules = set()
        self.header_paths = set()

    @classmethod
    def from_module(class_, mod):
        smod = class_(mod.name)
        public_ipath = set()
        public_req = set()
        private_req = set()
        def add_group(group, ipaths, reqs):
            ipaths = ipaths.union(p.path for p in group.header_paths)
            public_ipath.update(p.path for p in group.header_paths
                                if p.public)
            reqs = reqs.union(m.module for m in group.requirements)
            private_req.update(m.module for m in group.requirements)
            public_req.update(m.module for m in group.requirements
                              if m.public)
            smod.sources.extend(
                Source(src.path, src.type, ipaths, reqs)
                for src in group.sources)
            for subgroup in group.groups:
                add_group(subgroup, ipaths, reqs)
        add_group(mod.group, frozenset(), frozenset())
        smod.public_modules.update(public_req)
        smod.private_modules.update(private_req)
        smod.header_paths.update(public_ipath)
        return smod

    def dump(self):
        print('Source module {}'.format(self.name))
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
    __slots__ = ['name', 'sources', 'deps']
    is_target = False
    def __init__(self, name, sources, deps):
        self.name = name
        self.sources = sources
        self.deps = deps

def resolve_sources(build):
    """Resolve all dependencies among source modules.

    This replaces all source modules with flat source modules.  It
    also creates a list of all source files in the build.
    """
    dep_private = {}
    dep_public = {}
    modules = {}
    for mod in build.modules.values():
        if isinstance(mod, SourceModule):
            modules[mod.name] = mod
            dep_public[mod.name] = set(mod.public_modules)
            dep_private[mod.name] = set(mod.private_modules)
            dep_public[mod.name].add(mod.name)
            dep_private[mod.name].add(mod.name)
    for k in modules:
        for i in modules:
            for j in modules:
                if k in dep_private[i] and j in dep_private[k]:
                    dep_private[i].add(j)
                if k in dep_public[i] and j in dep_public[k]:
                    dep_public[i].add(j)
    intern_srcmods = {}
    intern_ipaths = {}
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
            for srcmod in srcmods:
                try:
                    m = modules[srcmod]
                except KeyError:
                    pass
                else:
                    ipaths.update(m.header_paths)
            srcmods.difference_update(modules)
            ipaths = tuple(sorted(ipaths))
            ipaths = intern_ipaths.setdefault(ipaths, ipaths)
            srcmods = tuple(sorted(srcmods))
            srcmods = intern_srcmods.setdefault(srcmods, srcmods)
            src2 = Source(src.path, src.type, ipaths, srcmods)
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
        nmod = FlatSourceModule(mname, msrc, mdeps)
        nmods[mname] = nmod
    build.modules.update(nmods)
    build.sources = sources
