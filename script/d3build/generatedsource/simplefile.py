# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import GeneratedSource

class SimpleFile(GeneratedSource):
    """A simple generated source with no dependencies."""
    __slots__ = ['target', 'contents']

    def __init__(self, target, contents):
        self.target = target
        self.contents = contents

    @property
    def is_binary(self):
        return isinstance(self.contents, bytes)

    def write(self, fp):
        """Write the data to a file."""
        fp.write(self.contents)
