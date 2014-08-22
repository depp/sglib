# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import GeneratedSource

class TemplateFile(GeneratedSource):
    __slots__ = ['source', 'tvars']

    @property
    def deps(self):
        return (self.source,)

    def write(self, fp, cfg):
        from build import template
        with open(cfg.native_path(self.source), encoding='UTF-8') as infp:
            srcdata = infp.read()
        fp.write(template.expand(srcdata, self.tvars))

    @classmethod
    def parse(class_, build, mod, external):
        tvars = {}
        for k in mod.info:
            if k.startswith('var.'):
                tvars[k[4:]] = mod.info.get_string(k)
        return class_(
            target=mod.info.get_path('target'),
            source=mod.info.get_path('source'),
            tvars=tvars,
        )