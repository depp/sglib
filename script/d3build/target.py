# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .error import ConfigError

class BaseTarget(object):
    """Base class for all target buildsystems."""
    __slots__ = [
        # List of all generated sources.
        'generated_sources',
        # List of errors that occurred while creating targets.
        'errors',
    ]

    def __init__(self):
        self.generated_sources = []
        self.errors = []

    @property
    def run_srcroot(self):
        """The path root of the source tree, at runtime."""
        raise NotImplementedError('must be implemented by subclass')

    def _add_module(self, module):
        """Add a module's dependencies and errors to the target.

        Returns True if the module is clean, False if the module has
        errors.
        """
        for source in module.generated_sources:
            self.add_generated_source(source)
        for error in module.errors:
            if not any(x is error for x in self.errors):
                self.errors.append(error)
        return not module.errors

    def add_generated_source(self, source):
        """Add a generated source to the build system."""
        if not any(x is source for x in self.generated_sources):
            self.generated_sources.append(source)

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

    def finalize(self):
        """Write the build system files."""
        remaining = set(source.target for source in self.generated_sources
                        if not source.is_regenerated_only)
        while remaining:
            advancing = False
            for source in self.generated_sources:
                if (source.target not in remaining or
                    remaining.intersection(source.dependencies)):
                    continue
                print('Creating {}...'.format(source.target))
                source.regen()
                advancing = True
                remaining.discard(source.target)
            if not advancing:
                raise ConfigError('circular dependency in generated sources')
