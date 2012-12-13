#!/bin/sh
# This is an example configuration script for an SGLib project
# Copy this file into your project, then adjust the path to init.py
for p in python2.7 python2.6 python2.5 python2 python ; do
    PYTHON=`which $p`
    if test -n "$PYTHON" ; then
        break
    fi
done
if test -z "$PYTHON" ; then
    echo 'error: could not find Python executable' 1>&2
    exit 1
fi
exec "$PYTHON" sglib/scripts/init.py config "$@"
