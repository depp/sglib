# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import multiprocessing
import os
import subprocess
from ..error import ConfigError

class ConfigureMake(object):
    """Target which is built using an external configure and make script."""
    __slots__ = [
        # A name for the package.
        'target',
        # A list of package dependencies.
        'dependencies',
        # Path to the source directory.
        'srcdir',
        # Path to the build directory.
        'builddir',
        # Path to the installation directory.
        'destdir',
        # Additional command-line arguments.
        'args',
    ]

    def __init__(self, build, name, path, *, dependencies=None, args=None):
        self.target = name
        self.dependencies = dependencies or []
        self.srcdir = path
        self.builddir = build.external_builddir(name)
        self.destdir = build.external_destdir()
        self.args = args or []

    @property
    def is_regenerated(self):
        return False

    @property
    def is_regenerated_always(self):
        return False

    @property
    def is_regenerated_only(self):
        return False

    def regen(self):
        srcdir = os.path.relpath(self.srcdir, self.builddir)
        cmd = [os.path.join(srcdir, 'configure')]
        cmd.extend(self.args)
        os.makedirs(self.builddir, exist_ok=True)

        status = self._read_status()
        try:
            if status not in ('config', 'build', 'install'):
                subprocess.check_call(cmd, cwd=self.builddir)
                self._write_status('config')
            if status not in ('build', 'install'):
                subprocess.check_call(
                    ['make', '-j{}'.format(multiprocessing.cpu_count())],
                    cwd=self.builddir)
                self._write_status('build')
            if status != 'install':
                subprocess.check_call(
                    ['make', 'install',
                     'DESTDIR=' + os.path.abspath(self.destdir)],
                    cwd=self.builddir)
                self._write_status('install')
        except subprocess.CalledProcessError:
            raise ConfigError('Could not build {}'.format(self.target))

    def _status_path(self):
        return os.path.join(self.builddir, 'd3build_status.txt')

    def _read_status(self):
        try:
            with open(self._status_path(), 'r') as fp:
                return fp.read().strip()
        except FileNotFoundError:
            return ''

    def _write_status(self, status):
        with open(self._status_path(), 'w') as fp:
            fp.write(status)
