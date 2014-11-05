# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .shell import escape, get_output
from .error import ConfigError
from .log import Log, logfile
import sys
import os

# Marker for detecting circular module dependencies
_circular = object()

class Build(object):
    __slots__ = [
        # The path to the configuration script.
        'script',
        # List of specifications for optional flags.
        'options',
        # The configuration options from the command line.
        'config',
        # Cached build variables.
        'cache',
        # Path to bundled packages.
        'package_search_path',
        # Dictionary mapping {varname: value}.
        'variables',
        # List of all variables as (varname, value).
        'variable_list',
        # Set of unused variables.
        'variable_unused',
        # The build system target.
        'target',
    ]

    @classmethod
    def run(class_, *, name, build, sources,
            options=[], adjust_config=None, package_search_path=None):
        """Main entry point for the configuration script."""

        import pickle
        args = sys.argv
        action = args[1] if len(args) >= 2 else None
        if action == '--action-regenerate':
            with open('config.dat', 'rb') as fp:
                gensources = pickle.load(fp)[1]
            if len(args) != 3:
                print('Invalid arguments', file=sys.stderr)
                sys.exit(1)
            try:
                source = gensources[args[2]]
            except KeyErorr:
                print('Unknown generated source file: {}'.format(args[2]))
                sys.exit(1)
            source = pickle.loads(source)
            source.regen()
            return
        elif action == '--action-reconfigure':
            with open('config.dat', 'rb') as fp:
                args = pickle.load(fp)[0]

        from .config import Config
        config = Config.parse_config(options=options, args=args[1:])

        with Log('config.log', config.verbosity):
            print(
                'Arguments:',
                ' '.join(escape(x) for x in args[1:]),
                file=logfile(2))
            if adjust_config is not None:
                adjust_config(config)
            config.dump(file=logfile(1))

            obj = class_()
            obj.script = args[0]
            obj.options = list(options)
            obj.config = config
            obj.cache = {}
            obj.package_search_path = package_search_path
            obj.variables = {}
            obj.variable_list = []
            obj.variable_unused = set()
            for vardef in config.variables:
                i = vardef.find('=')
                if i < 0:
                    raise UserError(
                        'invalid variable syntax: {!r}'.format(vardef))
                varname = vardef[:i]
                value = vardef[i+1:]
                obj.variables[varname] = value
                obj.variable_list.append((varname, value))
                obj.variable_unused.add(varname)

            if config.target == 'gnumake':
                from .gnumake import target
                target = target.GnuMakeTarget
            elif config.target == 'xcode':
                from .xcode import target
                target = target.XcodeTarget
            elif config.target == 'msvc':
                from .msvc import target
                target = target.VisualStudioTarget
            else:
                raise ConfigError(
                    'unknown target: {}'.format(config.target))
            obj.target = target(obj, name)
            build(obj)
            if obj.target.errors:
                log = logfile(0)
                print('Configuration failed.', file=log)
                for error in obj.target.errors:
                    print(file=log)
                    for line in error.splitlines():
                        print(line, file=log)
                sys.exit(1)
            if obj.variable_unused:
                print('Warning: unused variables:',
                      ' '.join(sorted(obj.variable_unused)),
                      file=logfile(0))
            gensources = {
                s.target: pickle.dumps(s, protocol=pickle.HIGHEST_PROTOCOL)
                for s in obj.target.generated_sources
            }
            with open('config.dat', 'wb') as fp:
                pickle.dump((args, gensources), fp)
            obj.target.finalize()

    def get_variable(self, name, default):
        """Get one of build variables."""
        value = self.variables.get(name, default)
        self.variable_unused.discard(name)
        return value

    def get_variable_bool(self, name, default):
        """Get one of the build variables as a boolean."""
        value = self.variables.get(name, None)
        if value is None:
            return default
        self.variable_unused.discard(name)
        uvalue = value.upper()
        if uvalue in ('YES', 'TRUE', 'ON', 1):
            return True
        if uvalue in ('NO', 'FALSE', 'OFF', 0):
            return False
        raise ConfigError('invalid value for {}: expecting boolean'
                          .format(name))

    def check_platform(self, platforms):
        """Throw an exception if the platform is not in the given set."""
        if isinstance(platforms, str):
            if self.config.platform == platforms:
                return
        else:
            if self.config.platform in platforms:
                return
            platforms = ', '.join(platforms)
        raise ConfigError('only valid on platforms: {}'.format(platforms))

    def find_package(self, pattern, *, varname=None):
        """Find a package matching the given pattern (a regular expression).

        Packages are directories stored in the package search path.  A
        target can use dependencies stored in the package search path
        without requiring the package to be installed on the
        developer's system or requiring the package to be integrated
        into the project repository.

        Returns the path to a directory in the package search path
        matching the given pattern.  If varname is not None, then it
        names a variable which can be used to specify the path to the
        given package, pre-empting the search process.
        """
        if varname is not None:
            value = self.get_variable(varname, None)
            if value is not None:
                return value
        if self.package_search_path is None:
            raise ConfigError('package_search_path is not set')
        try:
            filenames = os.listdir(self.package_search_path)
        except FileNotFoundError:
            raise ConfigError('package search path does not exist: {}'
                              .format(self.package_search_path))
        import re
        regex = re.compile(pattern)
        results = [fname for fname in filenames if regex.match(fname)]
        if not results:
            raise ConfigError('could not find package matching /{}/'
                              .format(pattern))
        if len(results) > 1:
            raise ConfigError('found multiple libraries matching /{}/: {}'
                              .format(pattern, ', '.join(results)))
        return os.path.join(self.package_search_path, results[0])

    def find_framework(self, name):
        """Find an OS X framework.

        Returns the framework's path.
        """
        if self.config.platform != 'osx':
            raise ConfigError('frameworks not available on this platform')
        try:
            home = os.environ['HOME']
        except KeyError:
            raise ConfigError('missing HOME environment variable')
        dir_paths = [
            os.path.join(home, 'Library/Frameworks'),
            '/Library/Frameworks',
            '/System/Library/Frameworks']
        framework_name = name + '.framework'
        for dir_path in dir_paths:
            try:
                fnames = os.listdir(dir_path)
            except FileNotFoundError:
                continue
            if framework_name in fnames:
                return os.path.join(dir_path, framework_name)
        raise ConfigError('could not find framework {}'
                          .format(framework_name))

    def external_builddir(self, name):
        """Get the build directory for an external package."""
        return os.path.join(self.package_search_path, 'build', name)

    def external_destdir(self):
        """Get the installation directory for external packages."""
        return os.path.join(self.package_search_path, 'build', 'products')

    def pkg_config(self, spec):
        """Return flags from the pkg-config tool."""
        cmdname = 'pkg-config'
        flags = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = get_output(
                [cmdname, '--silence-errors', arg, spec])
            if retcode:
                stdout, retcode = get_output(
                    [cmdname, '--print-errors', '--exists', spec],
                    combine_output=True)
                raise ConfigError(
                    '{} failed (spec: {})'.format(cmdname, spec),
                    details=stdout)
            flags[varname] = stdout.split()
        flags['CXXFLAGS'] = flags['CFLAGS']
        return flags

    def sdl_config(self, version):
        """Return flags from the sdl-config tool."""
        if version == 1:
            cmdname = 'sdl-config'
        elif version == 2:
            cmdname = 'sdl2-config'
        else:
            raise ValueError('unknown SDL version: {!r}'.format(version))
        flags = {}
        for varname, arg in (('LIBS', '--libs'), ('CFLAGS', '--cflags')):
            stdout, stderr, retcode = get_output([cmdname, arg])
            if retcode:
                raise ConfigError('{} failed'.format(cmdname), details=stderr)
            flags[varname] = stdout.split()
        flags['CXXFLAGS'] = flags['CFLAGS']
        return flags
