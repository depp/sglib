# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import multiprocessing
import os
import subprocess
from ..error import ConfigError

class ExternalBuildParameters(object):
    """Parameters for invocation of an external build system."""
    __slots__ = ['builddir', 'destdir', 'cflags', 'ldflags']

    def __init__(self, *, builddir, destdir, cflags='', ldflags=''):
        self.builddir = builddir
        self.destdir = destdir
        self.cflags = cflags
        self.ldflags = ldflags

    def run(self, *args, **kw):
        subprocess.check_call([])

class ConfigureMake(object):
    """Target which is built using an external configure and make script."""
    __slots__ = ['path', 'shared', 'static']

    def __init__(self, path, *, shared=False, static=True):
        self.path = path
        self.shared = shared
        self.static = static

    @staticmethod
    def enable_disable(name, flag):
        return ('--enable-' if flag else '--disable-') + name

    def configure_flags(self, params):
        """Get the flags for the configure script."""
        flags = [
            '--prefix=/',
            self.enable_disable('static', self.static),
            self.enable_disable('shared', self.shared),
        ]
        if params.cflags:
            flags.append('CFLAGS=' + params.cflags)
        if params.ldflags:
            flags.append('LDFLAGS=' + params.ldflags)
        return flags

    def build(self, params):
        """Build the dependency, and return True if successful."""
        srcdir = os.path.relpath(self.path, params.builddir)
        cmd = [os.path.join(srcdir, 'configure')]
        cmd.extend(self.configure_flags(params))
        os.makedirs(params.builddir, exist_ok=True)

        try:
            makefile = os.path.join(params.builddir, 'Makefile')
            if not os.path.isfile(makefile):
                subprocess.check_call(cmd, cwd=params.builddir)
            subprocess.check_call(
                ['make', '-j{}'.format(multiprocessing.cpu_count())],
                cwd=params.builddir)
            subprocess.check_call(
                ['make', 'install',
                 'DESTDIR=' + os.path.abspath(params.destdir)],
                cwd=params.builddir)
        except subprocess.CalledProcessError:
            return False
