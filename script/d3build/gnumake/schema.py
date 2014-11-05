# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..schema import Schema, SchemaBuilder

_VARIABLES = (SchemaBuilder()
    .string('CC', 'The C compiler')
    .string('CXX', 'The C++ compiler')
    .list  ('CPPFLAGS', 'C and C++ preprocessor flags')
    .list  ('CFLAGS', 'C compilation flags')
    .list  ('CXXFLAGS', 'C++ compilation flags')
    .list  ('CWARN', 'C warning flags')
    .list  ('CXXWARN', 'C++ warning flags')
    .list  ('LDFLAGS', 'Linker flags')
    .list  ('LIBS', 'Linker flags specifying libraries')
    .bool  ('WERROR', 'Treat warnings as errors')
).value()

_FLAGS = {f: f for f in ['CFLAGS', 'CXXFLAGS', 'LIBS']}

class GnuMakeSchema(Schema):
    """Build variable schema for the GNU Make target."""
    __slots__ = ['base']

    def __init__(self):
        configs = ['Debug', 'Release']
        super(GnuMakeSchema, self).__init__(
            variables=_VARIABLES,
            archs=['Native'],
            configs=configs,
            variants=configs,
            flags=_FLAGS)

    def get_variants(self, configs=None, archs=None):
        """Get a list of variants."""
        if archs is not None:
            raise ValueError(
                'this target does not support per-arch variables')
        if configs is None:
            return self.variants
        for config in configs:
            if config not in self.configs:
                raise ValueError(
                    'unknown configuration: {!r}'.format(config))
        return configs
