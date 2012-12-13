#!/bin/sh
# This is an example configuration script for an SGLib project
# Copy this file into your project, then adjust the path to init.py
if test -z "$PYTHON" ; then
    PYTHON=`which python3`
fi
if test -z "$PYTHON" ; then
    echo 'error: could not find Python 3 executable' 1>&2
    exit 1
fi
exec "$PYTHON" sglib/scripts/init.py config "$@"
