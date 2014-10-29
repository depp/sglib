# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.generatedsource import GeneratedSource, NOTICE
from d3build.shell import escape
import os

RUN_SCRIPT = '''\
#!/bin/sh
# {notice}
exe={exe}
name={name}
if test ! -x "$exe" ; then
  cat 1>&2 <<EOF
error: $exe does not exist
Did you remember to run 'make'?
EOF
  exit 1
fi
case "$1" in
  --gdb)
    shift
    exec gdb --args "$exe" {args} "$@"
    ;;
  --valgrind)
    shift
    exec valgrind -- "$exe" {args} "$@"
    ;;
  --help)
    cat <<EOF
Usage: $name [--help | --gdb | --valgrind] [options...]
EOF
    exit 0
    ;;
  *)
    exec "$exe" {args} "$@"
    ;;
esac
'''

class RunScript(GeneratedSource):
    __slots__ = ['_target', 'name', 'path', 'args']

    def __init__(self, target, name, path, args):
        self._target = target
        self.name = name
        self.path = path
        self.args = args

    @property
    def is_regenerated_only(self):
        return True

    @property
    def is_executable(self):
        return True

    @property
    def dependencies(self):
        return [self.path]

    @property
    def target(self):
        return self._target

    def write(self, fp):
        fp.write(RUN_SCRIPT.format(
            notice=NOTICE,
            exe=escape(self.path),
            name=escape(self.name),
            args=' '.join(escape(arg) for arg in self.args)))
