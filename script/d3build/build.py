# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import sys
from .shell import escape
from .error import ConfigError
from .module import ConfiguredModule
from .log import Log, logfile

# Marker for detecting circular module dependencies
_circular = object()

class Build(object):
    __slots__ = [
        # List of specifications for optional flags.
        'options',
        # The configuration options from the command line.
        'config',
        # The build environment.
        'env',
        # The build system target.
        'target',
        # The configuration script.
        'script',
        # Dictionary mapping module ids to configured modules.
        '_modules',
        # List of missing packages.
        '_missing_packages',
        # Indicates that the build system has recorded an error.
        '_has_error',
    ]

    def __init__(self):
        self._modules = {}
        self._missing_packages = []
        self._has_error = False

    @classmethod
    def run(class_, *, name, build, sources,
            options=[], adjust_config=None):
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
            obj.options = list(options)
            obj.config = config
            if config.target == 'gnumake':
                from .environment import nix
                env = nix.NixEnvironment
                from .target import gnumake
                target = gnumake.GnuMakeTarget
            elif config.target == 'xcode':
                from .environment import xcode
                env = xcode.XcodeEnvironment
                from .target import xcode
                target = xcode.XcodeTarget
            else:
                raise ConfigError(
                    'unknown target: {}'.format(config.target))

            obj.env = env(config)
            obj.env.dump(file=logfile(1))
            obj.target = target(name, args[0], config, obj.env)
            obj.script = args[0]
            try:
                build(obj)
            except ConfigError as ex:
                ex.write(logfile(0), indent='  ')
                obj._has_error = True
            if obj._has_error:
                print('Configuration failed.', file=logfile(0))
                sys.exit(1)
            if obj.env.variable_unused:
                print('Warning: unused variables:',
                      ' '.join(sorted(obj.env.variable_unused)),
                      file=logfile(0))
            gensources = {
                s.target: pickle.dumps(s, protocol=pickle.HIGHEST_PROTOCOL)
                for s in obj.target.generated_sources
            }
            with open('config.dat', 'wb') as fp:
                pickle.dump((args, gensources), fp)
            obj.target.finalize()

    def get_module(self, module):
        """Get the configured and flattened version of a module."""
        return self._configure_module(module).flatten()

    def _configure_module(self, module):
        """Configure an individual module."""
        try:
            result = self._modules[id(module)]
        except KeyError:
            pass
        else:
            if result is _circular:
                raise UserError('circular module dependency')
            return result
        self._modules[id(module)] = _circular
        try:
            m = module._configure(self)
        except ConfigError as ex:
            ex.write(logfile(0), indent='  ')
            self._has_error = True
            m = ConfiguredModule()
            m.sources = []
            m.public = []
            m.private = []
            m.dependencies = []
            m.has_error = True
        self._modules[id(module)] = m
        return m
