# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import platform, io
from .error import ConfigError
from .module import Module
from .log import logfile, Feedback

# There is a certain amount of guessing here...
_PACKAGE_SYSTEMS = [
    ('linux:debian linux:ubuntu linux:mint',
     'deb', 'sudo apt-get install {}'),

    ('linux:fedora linux:redhat linux:centos',
     'rpm', 'sudo yum install {}'),

    ('linux:gentoo',
     'gentoo', 'sudo emerge {}'),

    ('linux:arch',
     'arch', 'sudo pacman -S {}'),
]

_PACKAGE_SYSTEM = None

def get_package_system():
    """Get the package sytem which this platform uses."""
    global _PACKAGE_SYSTEM
    if _PACKAGE_SYSTEM is None:
        if platform.system() == 'Linux':
            distro = platform.linux_distribution(
                full_distribution_name=False)[0]
            name = 'linux:' + distro.lower()
        else:
            name = None
        for names, key, helpstr in _PACKAGE_SYSTEMS:
            names = names.split()
            if name in names:
                _PACKAGE_SYSTEM = key, helpstr
                break
        else:
            _PACKAGE_SYSTEM = None, None
    return _PACKAGE_SYSTEM

class ExternalPackage(object):
    """A function which gets a module which wraps an external package.

    There is a list of functions for getting the package's
    configuration.  If a function raises a ConfigError, the next
    function is called.  If one of the functions returns successfully,
    then the result is cached and returned.  If none of the functions
    return successfully, then a module is returned with instructions
    for the user to install the package.

    The functions should return (msg, module) where msg is the message
    to display to the user, or None for the default message which is
    'yes'.
    """
    __slots__ = ['_funcs', '_args', 'name', 'packages', 'uri']

    def __init__(self, funcs, *, args=(), name, packages=None, uri=None):
        self._funcs = funcs
        self._args = args
        self.name = name
        self.packages = packages
        self.uri = uri

    def __call__(self, build, *arg):
        key = (self.name,) + arg
        args = self._args + arg
        try:
            return build.cache[key]
        except KeyError:
            pass
        with Feedback('Checking for {}...'.format(self.name)) as fb:
            for func in self._funcs:
                try:
                    msg, result = func(build, *args)
                except ConfigError as ex:
                    ex.write(logfile(2), indent='  ')
                else:
                    fb.write(msg)
                    build.cache[key] = result
                    return result
            fb.write('no')
        err = self.get_error()
        mod = Module()
        mod.errors.append(mod)
        build.cache[key] = mod
        return mod

    def get_error(self):
        """Get the error for when this module's configuration fails."""
        fp = io.StringIO()
        msg = '{} could not be found'.format(self.name)
        if self.uri is not None:
            print('Project website: {}'.format(self.uri), file=fp)
        if self.packages is not None:
            key, helpstr = get_package_system()
            try:
                pkg = self.packages[key]
            except KeyError:
                pass
            else:
                print('To install, run:', helpstr.format(pkg), file=fp)
