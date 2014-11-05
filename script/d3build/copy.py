# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
class CopyFile(object):
    """Target which copies a file."""
    __slots__ = ['source', 'target']

    def __init__(self, source, target):
        self.source = source
        self.target = target
