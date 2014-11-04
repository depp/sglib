# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .error import ConfigError, UserError
from .schema import Variables

class SourceFile(object):
    """An individual source file with its build variables."""
    __slots__ = ['path', 'variables', 'sourcetype', 'external']

    def __init__(self, path, variables, sourcetype, external):
        self.path = path
        self.variables = variables
        self.sourcetype = sourcetype
        self.external = bool(external)

class Module(object):
    """A collection of source files and build settings."""
    __slots__ = [
        # The schema for build variables.
        'schema',
        # List of source files (SourceFile objects) in the module.
        'sources',
        # List of generated source files.
        'generated_sources',
        # Public variables for this module.
        '_public',
        # List of public variable sets inherited by source files dependent on
        # this module.  Contains _public.
        '_public_varsets',
        # List of all variable sets used by the module.  Contains _public.
        '_all_varsets',
        # List of errors in this module.
        'errors',
    ]

    def __init__(self, schema):
        self.schema = schema
        self.sources = []
        self.generated_sources = []
        self.errors = []
        self._public = {}
        self._public_varsets = [self._public]
        self._all_varsets = [self._public]

    def variables(self):
        """Get the build variables for this module."""
        return Variables(self.schema, self._all_varsets)

    def add_source(self, path, sourcetype=None, external=True):
        """Add a single source file with no module dependencies."""
        src = SourceFile(
            path,
            Variables(self.schema, []),
            sourcetype,
            external)
        self.sources.append(src)
        return self

    def add_generated_source(self, source):
        """Add a generated source file to the module."""
        self.generated_sources.append(source)
        return self

    def add_sources(self, sources, tagdefs):
        """Add source files to the module.

        Each source file is associated with a set of tags.  The tag
        definitions map each tag to either None or a list of modules.
        If a tag is mapped to None, then sources with that tag are not
        included in the module.  If a tag is mapped to a list of
        modules, then source files with those tags are dependent on
        those modules.

        Four tags have special meaning.  The 'public' tag contains
        dependencies which are propagated to files which depend on
        this module.  The 'private' tag is ignored if it is missing.
        The 'external' tag marks source files that should be compiled
        with warnings disabled.  The 'exclude' tag marks source files
        that should be excluded entirely.
        """

        def check_tags():
            """Check that the tag definitions match the source files."""
            special = 'public', 'private', 'external', 'exclude'
            srctags = set()
            for source in sources:
                srctags.update(source.tags)
            srctags.difference_update(special)
            deftags = set(tagdefs)
            deftags.difference_update(special)
            extras = deftags.difference(srctags)
            missing = srctags.difference(deftags)
            if extras or missing:
                msgs = []
                if missing:
                    msgs.append(
                        'missing tags: {}'.format(', '.join(sorted(missing))))
                if extras:
                    msgs.append(
                        'extra tags: {}'.format(', '.join(sorted(extras))))
                raise UserError('; '.join(msgs))
            for tag in ('public', 'private'):
                if tagdefs.get(tag, True) is None:
                    raise UserError('"{}" tag should not be None'.format(tag))
            if 'external' in tagdefs:
                raise UserError('"external" tag should not be defined')
            for tag, tagdef in tagdefs.items():
                if tagdef is None:
                    continue
                if not isinstance(tagdef, list):
                    raise UserError('"{}" tag definition is not a list'
                                    .format(tag))
                if not all(isinstance(x, Module) for x in tagdef):
                    raise UserError('"{}" tag contains invalid item')

        def resolve_varsets(tags):
            """Resolve a list of tags to a list of variable sets."""
            if 'exclude' in tags:
                return None
            varsets = []
            for tag in tags:
                try:
                    modules = tagdefs[tag]
                except KeyError:
                    continue
                if modules is None:
                    return None
                for module in modules:
                    for varset in module._public_varsets:
                        if not varset:
                            continue
                        if not any(x is varset for x in varsets):
                            varsets.append(varset)
            return varsets

        def add_sources():
            """Add tagged source files to this module."""
            all_tags = set()
            schema = self.schema
            for source in sources:
                source_varsets = resolve_varsets(source.tags)
                if source_varsets is None:
                    continue
                all_tags.update(source.tags)
                self.sources.append(SourceFile(
                    source.path,
                    Variables(schema, source_varsets),
                    source.sourcetype,
                    'external' in source.tags))
            all_tags.add('public')
            for tag in all_tags:
                modules = tagdefs.get(tag)
                if not modules:
                    continue
                public = tag == 'public'
                for module in modules:
                    self.add_module(module, public=public)

        check_tags()
        add_sources()
        return self

    def add_variables(self, variables, *, configs=None, archs=None):
        """Add public variables to this module."""
        schema = self.schema
        variants = schema.get_variants(configs, archs)
        for varname, value2 in variables.items():
            vardef = schema.get_variable(varname)
            for variant in variants:
                try:
                    value1 = self._public[variant, varname]
                except KeyError:
                    value = value2
                else:
                    value = vardef.combine([value1, value2])
                self._public[variant, varname] = value
        return self

    def add_module(self, module, *, public=True):
        """Add a module dependency on another module."""
        if public:
            for varset in module._public_varsets:
                if not varset:
                    continue
                if not any(x is varset for x in self._public_varsets):
                    self._public_varsets.append(varset)
        for varset in module._all_varsets:
            if not varset:
                continue
            if not any(x is varset for x in self._all_varsets):
                self._all_varsets.append(varset)
        for source in module.sources:
            if not any(x is source for x in self.sources):
                self.sources.append(source)
        for source in module.generated_sources:
            if not any(x is source for x in self.generated_sources):
                self.generated_sources.append(source)
        for error in module.errors:
            if not any(x is error for x in self.errors):
                self.errors.append(error)
        return self

    def add_define(self, definition, *, configs=None, archs=None):
        """Add a preprocessor definition."""
        raise NotImplementedError('must be implemented by subclass')

    def add_header_path(self, path, *,
                        configs=None, archs=None, system=False):
        """Add a header search path.

        If system is True, then the header path will be searched for
        include statements with angle brackets.  Otherwise, the header
        path will be searched for include statements with double
        quotes.  Not all targets support the distinction.
        """
        raise NotImplementedError('must be implemented by subclass')

    def add_library(self, path, *, configs=None, archs=None):
        """Add a library."""
        raise ConfigError('add_library not available on this target')

    def add_library_path(self, path, *, configs=None, archs=None):
        """Add a library search path."""
        raise ConfigError('add_library_path not available on this target')

    def add_framework(self, *, name=None, path=None,
                      configs=None, archs=None):
        """Add a framework.

        Either the name or the path should be specified (but not both).
        """
        raise ConfigError('add_framework not available on this target')

    def add_framework_path(self, path, *, configs=None, archs=None):
        """Add a framework search path."""
        raise ConfigError('add_framework_path not available')

    def add_pkg_config(self, spec, *, configs=None, archs=None):
        """Run the pkg-config tool and add the resulting settings."""
        raise ConfigError('add_pkg_config not available on this target')

    def add_sdl_config(self, version, *, configs=None, archs=None):
        """Run the sdl-config tool and add the resulting settings.

        The version should either be 1 or 2.
        """
        raise ConfigError('add_sdl_config not available on this target')

    def test_compile(self, source, sourcetype, *,
                     configs=None, archs=None, external=True, link=True):
        """Try to compile a source file.

        Returns True if successful, False otherwise.
        """
        raise ConfigError(
            'test_compile not available on this target')
