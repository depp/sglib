# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .error import ConfigError

class ExternalTarget(object):
    __slots__ = ['target', 'name', 'dependencies', 'destdir', 'builddir']
    def __init__(self, target, name, dependencies, destdir, builddir):
        self.target = target
        self.name = name
        self.dependencies = list(dependencies)
        self.destdir = destdir
        self.builddir = builddir
    def build(self):
        raise NotImplementedError('this should be implemented by subclass')

class BaseTarget(object):
    """Base class for all target buildsystems."""
    __slots__ = [
        # The environment.
        'env',
        # List of all generated sources.
        'generated_sources',
        # Dictionary mapping names to external targets.
        'external_targets',
    ]

    def __init__(self, name, script, config, env):
        self.env = env
        self.generated_sources = []
        self.external_targets = {}

    @property
    def run_srcroot(self):
        """The path root of the source tree, at runtime."""
        raise NotImplementedError('must be implemented by subclass')

    def external_target(self, obj, name, dependencies=[]):
        """Create an external target, without adding it to the build."""
        raise ConfigError('this target does not suport external targets')

    def project_reference(self, path):
        """Create build variables that reference another project."""
        raise ConfigError('project_reference not available on this target')

    def add_generated_source(self, source):
        """Add a generated source to the build system.

        Returns the path to the generated source.
        """
        self.generated_sources.append(source)
        return source.target

    def add_default(self, target):
        """Set a target to be a default target."""

    def add_executable(self, *, name, module, uuid=None, arguments=[]):
        """Create an executable target.

        Returns the path to the executable.
        """
        raise ConfigError('this target does not support executables')

    def add_application_bundle(self, *, name, module, info_plist,
                               arguments=[]):
        """Create an OS X application bundle target.

        Returns the path to the application bundle.
        """
        raise ConfigError(
            'this target does not support application bundles')

    def add_external_target(self, obj):
        """Add a target built with an external build system."""
        if not isinstance(obj, ExternalTarget):
            raise TypeError('external target must be ExternalTarget')
        if obj.name in self.external_targets:
            raise UserError('external build name conflict: {}'
                            .format(obj.name))
        self.external_targets[obj.name] = obj

    def external_build_path(self, obj):
        """Get the build path for an external target."""
        raise ConfigError('not implemented')

    def finalize(self):
        """Write the build system files."""
        self._build_external()
        self._build_generated()

    def _build_external(self):
        """Build external targets."""
        remaining = set(self.external_targets)
        while remaining:
            advancing = False
            for name in tuple(remaining):
                target = self.external_targets[name]
                if remaining.intersection(target.dependencies):
                    continue
                print('Building {}...'.format(name))
                target.build()
                advancing = True
                remaining.discard(name)
            if not advancing:
                raise ConfigError('circular dependency in external targets')

    def _build_generated(self):
        """Build generated sources."""
        for source in self.generated_sources:
            if source.is_regenerated_only:
                continue
            print('Creating {}...'.format(source.target))
            source.regen()
