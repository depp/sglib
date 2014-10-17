from ..error import ConfigError

class BaseEnvironment(object):
    """Base class for all configuration environments."""
    __slots__ = []

    def __init__(self, config):
        pass

    def pkg_config(self, spec):
        """Run the pkg-config tool and return the build variables."""
        raise ConfigError('pkg-config not available for this target')

    def sdl_config(self, version):
        """Run the sdl-config tool and return the build variables.

        The version should either be 1 or 2.
        """
        raise ConfigError('sdl-config not available for this target')

    def frameworks(self, flist):
        """Specify a list of frameworks to use."""
        raise ConfigError('frameworks not available on this target')

    def test_compile_link(self, source, sourcetype, base_varset, varsets):
        """Try different build variable sets to find one that works."""
        raise ConfigError(
            'compile and link tests not available for this target')
