# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import GeneratedSource

class LiteralFile(GeneratedSource):
    __slots__ = ['contents']
    is_regenerated = False

    def write(self, fp, cfg):
        fp.write(self.contents)

    @classmethod
    def parse(class_, build, mod, external):
        return class_(
            target=mod.info.get_path('target'),
            contents=mod.info.get_string('contents'),
        )
