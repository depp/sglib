from .environment import BaseEnvironment
from .variable import BuildVariables
from ..shell import get_output
from ..error import ConfigError

class NixEnvironment(BaseEnvironment):
    """A Unix-like configuration environment."""

    def pkg_config(self, spec):
        """Run the pkg-config tool and return the build variables."""
        cmdname = 'pkg-config'
        flags = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = get_output(
                [cmdname, '--silence-errors', arg, spec])
            if retcode:
                stdout, retcode = get_output(
                    [cmdname, '--print-errors', '--exists', arg, spec],
                    combine_output=True)
                raise ConfigError('{} failed'.format(cmdname), details=stdout)
            flags[varname] = stdout
        varset = BuildVariables.parse(flags)
        varset.CXXFLAGS = varset.CFLAGS
        return varset
