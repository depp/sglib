# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from build.error import ConfigError
from xml.sax.saxutils import escape
import re
VAR = re.compile(r'\$\{(\w+)\}', re.ASCII)

def expand(text, tvars):
    def repl(m):
        k = m.group(1)
        try:
            v = tvars[k]
        except KeyError:
            raise ConfigError('unknown variable in template: {}'
                              .format(m.group(0)))
        return escape(v)
    return VAR.sub(repl, text)
