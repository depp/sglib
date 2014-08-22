#!/bin/sh
# This is an example configuration script for an SGLib project
# Copy this file into your project, then adjust the path to config.py
if test -z "$PYTHON" ; then
    PYTHON=`which python3`
fi
if test -z "$PYTHON" ; then
    PYTHON=`which python`
fi
if test -z "$PYTHON" ; then
    echo 'error: could not find Python executable' 1>&2
    exit 1
fi
if "$PYTHON" -c 'import sys; sys.exit(sys.version_info[0] == 3)' ; then
    echo 'error: need Python 3.x' 1>&2
    exit 1
fi
srcdir=`dirname "$0"`
exec "$PYTHON" "$srcdir/script/config.py" config "$srcdir/project.xml" "$@"
