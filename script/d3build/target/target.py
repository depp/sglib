# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..error import ConfigError

class Target(object):
    """Base class for all target buildsystems."""
    __slots__ = [
        # List of all generated sources.
        'generated_sources',
    ]

    def __init__(self, name, script, config, env):
        pass

    def add_generated_source(self, source):
        """Add a generated source to the build system.

        Returns the path to the generated source.
        """
        return source.target

    def add_default(self, target):
        """Set a target to be a default target."""
        self._all.add(target)

    def add_executable(self, *, name, module, uuid=None):
        """Create an executable target.

        Returns the path to the executable.
        """
        raise ConfigError('this target does not support executables')

    def add_application_bundle(self, name, module, info_plist):
        """Create an OS X application bundle target.

        Returns the path to the application bundle.
        """
        raise ConfigError(
            'this target does not support application bundles')

    def finalize(self):
        """Write the build system files."""
        for source in self.generated_sources:
            if source.is_regenerated_only:
                continue
            with open(source.target, 'w') as fp:
                source.write(fp)
