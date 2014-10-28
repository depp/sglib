# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import GeneratedSource
import shutil

class CopyFile(GeneratedSource):
    """Target which copies a file."""
    __slots__ = ['source', 'target']

    def __init__(self, source, target):
        self.source = source
        self.target = target

    @property
    def dependencies(self):
        return [self.source]

    def rule(self):
        return 'Copy', [['cp', '--', self.source, self.target]]

    def regen(self):
        self.makedirs()
        shutil.copy(self.source, self.target)
