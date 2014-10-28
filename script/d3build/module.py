# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .error import ConfigError, UserError
from .source import SourceFile
from .log import Feedback

class _ModuleConfigurator(object):
    __slots__ = ['build', 'tagdefs', 'has_error', 'modules', '_module_ids']
    def __init__(self, build, tagdefs):
        self.build = build
        self.tagdefs = tagdefs
        self.has_error = False
        self.modules = []
        self._module_ids = set()
    def resolve_tags(self, tags):
        varsets = []
        varset_ids = set()
        module_ids = set()
        new_modules = []
        for tag in tags:
            if tag == 'exclude':
                return None
            tagdef = self.tagdefs.get(tag, True)
            if isinstance(tagdef, bool):
                if not tagdef:
                    return None
                continue
            if not isinstance(tagdef, list):
                raise TypeError(
                    'tag definitions must be list or bool (tag={})'
                    .format(tag))
            for item in tagdef:
                if isinstance(item, dict):
                    new_varsets = (item,)
                elif isinstance(item, Module):
                    if id(item) in module_ids:
                        continue
                    module_ids.add(id(item))
                    cfg_m = self.build._configure_module(item)
                    self.has_error = self.has_error or cfg_m.has_error
                    new_varsets = cfg_m.public
                    new_modules.append(cfg_m)
                else:
                    raise TypeError(
                        'tag definitions must contain variable sets '
                        'and modules (tag={})'.format(tag))
                for new_varset in new_varsets:
                    if id(new_varset) in varset_ids:
                        continue
                    varset_ids.add(id(new_varset))
                    varsets.append(new_varset)
        for new_module in new_modules:
            if id(new_module) in self._module_ids:
                continue
            self._module_ids.add(id(new_module))
            self.modules.append(new_module)
        return varsets

class Module(object):
    """A module in a larger object to be compiled.

    Modules consist of source files and tags.  During configuration,
    the tags are resolved to build environments.  A build environment
    can contain header search paths, libraries, build flags, module
    dependencies, and various other components.  Tags can have any
    name, each tag only has meaning within the module it is used.
    There are two special tags.  The 'private' tag applies to all
    source files in the module.  The 'public' tag applies to all
    source files in the module, as well as all source files which
    depend on the module.
    """
    __slots__ = []

    def _configure(self, build):
        int_sources = self.sources
        all_tags = set()
        for source in int_sources:
            all_tags.update(source.tags)
        ext_sources, tagdefs = self._get_configs(build)
        extras = (set(tagdefs).difference(all_tags)
                  .difference(['public', 'private']))
        missing = all_tags.difference(tagdefs)
        missing.discard('exclude')
        if extras or missing:
            raise UserError(
                'missing tag configurations: {}; extra tag configurations: {}'
                .format(', '.join(sorted(missing)),
                        ', '.join(sorted(extras))))
        cfg = _ModuleConfigurator(build, tagdefs)
        out = ConfiguredModule()
        out.sources = []
        sourcelists = [(int_sources, False), (ext_sources, True)]
        for (sources, external) in sourcelists:
            for source in sources:
                varsets = cfg.resolve_tags(
                    ('public', 'private') + source.tags)
                if varsets is None:
                    continue
                out.sources.append(
                    SourceFile(source.path, varsets,
                               source.sourcetype,external))
        out.public = cfg.resolve_tags(('public',)) or []
        out.private = cfg.resolve_tags(('private',)) or []
        out.dependencies = cfg.modules
        out.has_error = cfg.has_error
        return out

    def _get_configs(self, build):
        """Get the module external sources and tags.

        The default implementation returns no sources or tags.
        """
        return [], {}

class SourceModule(Module):
    """A module consisting of source code to be compiled."""
    __slots__ = ['_sources', '_tags_func']

    def __init__(self, *, sources=None, configure=None):
        self._sources = sources
        self._tags_func = configure

    @property
    def sources(self):
        if self._sources is None:
            return []
        return self._sources.sources

    def _get_configs(self, build):
        if self._tags_func is None:
            return [], {}
        return self._tags_func(build)

class ExternalModule(Module):
    """A module representing an external package.

    If configuration of an external package fails, information about
    the package is printed for the user.
    """
    __slots__ = ['name', '_tags_func', 'packages']

    def __init__(self, *, name, configure, packages):
        if not callable(configure):
            raise TypeError('configure is not callable')
        self.name = name
        self._tags_func = configure
        self.packages = packages

    @property
    def sources(self):
        return []

    def _get_configs(self, build):
        with Feedback('Checking for {}...'.format(self.name)) as fb:
            status, sources, varsets = self._tags_func(build)
            if status is not None:
                fb.write(status)
            return sources, varsets

class ConfiguredModule(object):
    """A module which has been configured."""
    __slots__ = ['sources', 'public', 'private', 'dependencies',
                 'has_error']

    def flatten(self):
        """Flatten a module, resolving all dependencies."""
        if self.has_error:
            return None
        mod = FlatModule()
        mod.sources = []
        mod.varsets = []
        varset_ids = set()
        mq = [self]
        mids = {id(self)}
        while mq:
            m = mq.pop()
            for dep in m.dependencies:
                if id(dep) in mids:
                    continue
                mids.add(id(dep))
                mq.append(dep)
            for source in m.sources:
                mod.sources.append(source)
                for varset in source.varsets:
                    if id(varset) in varset_ids:
                        continue
                    varset_ids.add(id(varset))
                    mod.varsets.append(varset)
            for varsets in (m.public, m.private):
                for varset in varsets:
                    if id(varset) in varset_ids:
                        continue
                    varset_ids.add(id(varset))
                    mod.varsets.append(varset)
        return mod

class FlatModule(object):
    """A module which has been configured and flattened."""
    __slots__ = ['sources', 'varsets']
