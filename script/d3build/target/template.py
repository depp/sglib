# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import re
from . import GeneratedSource

class TemplateFile(GeneratedSource):
    __slots__ = ['target', 'source', 'tvars']

    def __init__(self, target, source, tvars):
        self.target = target
        self.source = source
        self.tvars = tvars

    @property
    def dependencies(self):
        return (self.source,)

    def write(self, fp):
        """Write the generated source to the given file."""
        with open(self.source) as infp:
            srcdata = infp.read()
        output = re.sub(r'\$\{(\w+)\}', self._expand, srcdata)
        fp.write(output)

    def _expand(self, m):
        """Expand a single variable."""
        k = m.group(1)
        try:
            v = self.tvars[k]
        except KeyError:
            raise ConfigError('unknown variable in template: {}'
                              .format(m.group(0)))
        return v
