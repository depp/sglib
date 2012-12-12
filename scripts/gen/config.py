from __future__ import with_statement

from gen.path import Path
from gen.source import Source
from gen.error import ConfigError
import gen.project as project
import os
import sys

# TODO: After loading the project,
# make sure each tag refers to one module

def use_bundled_lib(proj, lib, lsrc, fname):
    root = Path(proj.lib_path, fname)
    sys.stderr.write('found bundled library: %s\n' % root.posix)

    lib.header_path = lsrc.header_path
    lib.define = lsrc.define
    lib.require = lsrc.require
    lib.cvar = lsrc.cvar
    lib.sources = [Source(Path(root, s.path), s.tags) for s in lsrc.sources]
    lib.use_bundled = True

def find_bundled_lib(proj, lib, libs):
    for lsrc in lib.bundled_versions:
        searchname = lsrc.libname
        for fname in libs:
            i = fname.rfind('-')
            if i >= 0:
                libname = fname[:i]
            else:
                libname = fname
            if searchname == libname:
                use_bundled_lib(proj, lib, lsrc, fname)
                return

def find_bundled_libs(proj, opt_dict):
    """Scan the bundled library folder for bundled libraries.

    This will fill in the module fields for every ExternalLibrary that
    can be bundled, if that library is found in the library folder.
    """

    if proj.lib_path is None:
        return

    libs = []
    for fname in os.listdir(proj.lib_path.native):
        npath = os.path.join(proj.lib_path.native, fname)
        if fname.startswith('.'):
            continue
        if not os.path.isdir(npath):
            continue
        try:
            Path(fname)
        except ValueError:
            sys.stderr.write('warning: ignoring %s\n' % npath)
            continue
        libs.append(fname)

    for m in proj.modules:
        if isinstance(m, project.ExternalLibrary) and m.bundled_versions:
            use_bundled = opt_dict['bundled_' + m.modid]
            if use_bundled is None:
                use_bundled = True
            if use_bundled:
                find_bundled_lib(proj, m, libs)

def trim_project(proj):
    """Remove unreferenced modules from the project."""
    mods, unsat = proj.closure(proj.targets())
    all_mods = set(proj.module_names)
    used_mods = set(mod.modid for mod in mods if mod.modid is not None)
    unused_mods = all_mods.difference(used_mods)
    for mid in unused_mods:
        del proj.module_names[mid]

def check_deps(proj):
    """Check for unsatisfied dependencies in the project."""
    mods, unsat = proj.closure(proj.targets())
    if unsat:
        sys.stderr.write(
            'warning: unknown module dependencies: %s\n' %
            ', '.join(sorted(unsat)))
    all_tags = set()
    used_tags = set()
    for m in proj.modules:
        if m.modid is not None:
            all_tags.add(m.modid)
        all_tags.update(f.modid for f in m.feature)
        for s in m.sources:
            used_tags.update(s.tags)
    unsat_tags = used_tags.difference(all_tags)
    if unsat_tags:
        sys.stderr.write(
            'warning: unknown source tags: %s\n' %
            ', '.join(sorted(unsat_tags)))

def parse_feature_args(proj, keys, p):
    import optparse
    ge = optparse.OptionGroup(p, 'Optional Features')
    gw = optparse.OptionGroup(p, 'Optional Packages')
    p.add_option_group(ge)
    p.add_option_group(gw)

    optlibs = set()
    for f in proj.features():
        default = True
        nm = f.modid.lower()
        desc = '%s feature' % f.modid if f.desc is None else f.desc
        var = 'enable_%s' % f.modid
        keys.append(var)
        ge.add_option(
            '--enable-%s' % nm,
            default=default,
            action='store_true',
            dest=var,
            help=('enable %s' % desc if not default
                  else optparse.SUPPRESS_HELP))
        ge.add_option(
            '--disable-%s' % nm,
            default=default,
            action='store_false',
            dest=var,
            help=('disable %s' % desc if default
                  else optparse.SUPPRESS_HELP))

        for i in f.impl:
            optlibs.update(i.require)

    del f

    for m in proj.modules:
        if not isinstance(m, project.ExternalLibrary):
            continue
        nm = m.modid.lower()
        desc = m.name if m.name is not None else '%s library' % m.modid
        opts = []
        if m.modid in optlibs:
            opts.append((nm, 'with_%s' % m.modid, desc))
        if m.bundled_versions:
            opts.append(('bundled-%s' % nm, 'bundled_%s' % m.modid,
                         '%s (bundled copy)' % desc))

        for argn, var, desc in opts:
            keys.append(var)
            gw.add_option(
                '--with-%s' % argn,
                default=None,
                action='store_true',
                dest=var,
                help='use %s' % desc)
            gw.add_option(
                '--without-%s' % argn,
                default=None,
                action='store_false',
                dest=var,
                help=optparse.SUPPRESS_HELP)

class ProjectConfig(object):
    """Project-wide configuration."""

    __slots__ = ['argv', 'project', 'opts', 'vars']

    def __init__(self):
        self.argv = None
        self.project = None
        self.opts = None
        self.vars = None

    def reconfig(self):
        import optparse
        import gen.xml as xml

        proj = xml.load('project.xml', Path())
        for modpath in proj.module_path:
            for fname in os.listdir(modpath.native):
                if fname.startswith('.') or not fname.endswith('.xml'):
                    continue
                mod = xml.load(os.path.join(modpath.native, fname), modpath)
                proj.add_module(mod)
        self.project = proj

        trim_project(proj)
        for tag in project.INTRINSICS:
            proj.add_module(project.Intrinsic(tag))
        check_deps(proj)

        p = optparse.OptionParser()
        keys = []
        parse_feature_args(proj, keys, p)

        opts, args = p.parse_args(self.argv)
        opt_dict = {}
        for k in keys:
            opt_dict[k] = getattr(opts, k)

        var_dict = {}
        targets = []
        for arg in args:
            i = arg.find('=')
            if i >= 0:
                var_dict[arg[:i]] = arg[i+1:]
            else:
                targets.append(arg)

        find_bundled_libs(proj, opt_dict)

        self.opts = opt_dict
        self.vars = var_dict
        if not targets:
            targets = ['default']
        return targets

    def is_enabled(self, featid):
        """Return the --enable state of the feature."""
        return self.opts['enable_' + featid]

    def is_with(self, modid):
        """Return the --with state of the library."""
        return self.opts.get('with_' + modid, None)

    def get_config(self, os):
        """Get the build configuration for the given os.

        Returns a BuildConfig object.
        """

        intrinsics = set(project.OS[os])

        dlist = self.project.defaults
        for m in self.project.modules:
            dlist.extend(m.defaults)

        # Get defaults
        enabled_variants = set()
        enabled_features = set()
        enabled_libs = set()
        for d in dlist:
            if d.os is not None and os not in d.os:
                continue
            if d.variants is not None:
                enabled_variants.update(d.variants)
            if d.features is not None:
                enabled_features.update(d.features)
            if d.libs is not None:
                enabled_libs.update(d.libs)

        # Override defaults with user settings
        features = list(self.project.features())
        for f in features:
            if self.is_enabled(f.modid):
                enabled_features.add(f.modid)
            else:
                enabled_features.discard(f.modid)
        user_libs = set()
        for modid in self.project.module_names:
            v = self.is_with(modid)
            if v is None:
                continue
            if v:
                enabled_libs.add(modid)
                user_libs.add(modid)
            else:
                enabled_libs.discard(modid)

        # Add platform intrinsics (you can't disable WINDOWS on Windows)
        enabled_libs.update(intrinsics)

        # Remove defaults that conflict with user preferences
        # So you can specify --with-x without needing --without-y
        for f in features:
            if f.modid not in enabled_features:
                continue
            has_user = False
            for i in f.impl:
                if user_libs.intersection(i.require):
                    has_user = True
            if not has_user:
                continue
            for i in f.impl:
                if not user_libs.intersection(i.require):
                    enabled_libs.difference_update(i.require)

        # Check for conflicts and unsatisfied dependencies
        needed_libs = set()
        for f in features:
            if f.modid not in enabled_features:
                continue
            impls = []
            for i in f.impl:
                if all(lib in enabled_libs for lib in i.require):
                    impls.append(i)
            if not impls:
                raise ConfigError(
                    'unsatisfied feature: %s' % f.modid.lower(),
                    'libraries required for this feature are disabled\n'
                    'use --disable-%s if you want to disable this feature\n'
                    % f.modid.lower())
            if len(impls) > 1:
                raise ConfigError(
                    'overly satisfied feature: %s' % f.modid.lower(),
                    'this feature is satisfied in multiple ways: %s\n'
                    'use fewer --with-lib options\n' %
                    (', '.join(' '.join(i.require) for i in impls)))
            needed_libs.update(impls[0].require)
        needed_libs.difference_update(intrinsics)

        cfg = BuildConfig()
        cfg.project = self.project
        cfg.projectconfig = self
        cfg.os = os
        cfg.variants = frozenset(enabled_variants)
        cfg.features = frozenset(enabled_features)
        cfg.libs = frozenset(needed_libs)
        cfg._calculate_targets()
        return cfg

class BuildConfig(object):
    """Configuration for a specific build."""

    __slots__ = ['project', 'projectconfig', 'os',
                 'variants', 'features', 'libs',
                 'targets']

    def dump(self):
        print 'os: %s' % self.os
        for attr in ('variants', 'features', 'libs'):
            print '%s: %s' % (attr, ' '.join(getattr(self, attr)))

    def _calculate_targets(self):
        self.targets = []
        for t in self.project.targets():
            for v in self.variants:
                vinst = []
                tags = set()
                mods = [t]
                q = list(mods)
                while q:
                    m = q.pop()
                    reqs = set()
                    reqs.update(m.require)
                    for f in m.feature:
                        if f.modid not in self.features:
                            continue
                        for i in f.impl:
                            if all(x in self.libs for x in i.require):
                                reqs.update(i.require)
                    for vv in m.variant:
                        if vv.varname == v:
                            reqs.update(vv.require)
                            vinst.append(vv)
                    reqs.difference_update(tags)
                    tags.update(reqs)
                    for modid in reqs:
                        m = self.project.module_names[modid]
                        q.append(m)
                        mods.append(m)
                assert len(vinst) == 1
                tags.update(self.features)
                tags.update(project.OS[self.os])
                bt = BuildTarget(t, vinst[0], mods, tags)
                self.targets.append(bt)

class BuildTarget(object):
    __slots__ = ['target', 'variant', 'modules', 'tags', 'tag_closure',
                 'tag_modules']

    def __init__(self, target, variant, modules, tags):
        self.target = target
        self.variant = variant
        self.modules = list(modules)
        self.tags = frozenset(tags)
        tag_map = {}
        tag_modules = set()
        for m in self.modules:
            if m.modid is None:
                continue
            tag_modules.add(m.modid)
            if m.require:
                tag_map[m.modid] = frozenset(m.require)
        tag_closure = {}
        for tag in tag_map:
            deps = set()
            ndeps = set(tag_map[tag])
            while ndeps:
                deps.update(ndeps)
                odeps = ndeps
                ndeps = set()
                for dtag in odeps:
                    try:
                        ndeps.update(tag_map[dtag])
                    except KeyError:
                        pass
                ndeps.difference_update(deps)
            deps.discard(tag)
            if deps:
                tag_closure[tag] = frozenset(deps)
        self.tag_closure = tag_closure
        self.tag_modules = tag_modules

    def dump(self):
        print 'target:', self.target
        print 'variant:', self.variant.varname
        print 'tags:', ' '.join(self.tags)

    def sources(self):
        usedtags = set()
        for m in self.modules:
            base = set(m.require)
            if m.modid is not None:
                base.add(m.modid)
            for s in m.sources:
                tags = base.union(s.tags)
                for tag in tuple(tags):
                    try:
                        tags.update(self.tag_closure[tag])
                    except KeyError:
                        pass
                if tags.issubset(self.tags):
                    tags.intersection_update(self.tag_modules)
                    usedtags.update(tags)
                    yield Source(s.path, tuple(sorted(tags)))
