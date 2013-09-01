# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import Object
from . import source
import uuid

class Target(Object):
    __slots__ = ['uuid', 'filename']

    def get_uuid(self, cfg):
        if self.uuid is None:
            self.uuid = uuid.uuid4()
            cfg.warn('target has no UUID, generated: {}'.format(self.uuid))
        return self.uuid

def parse_args(info):
    args = []
    pfx = 'default-args.'
    for k, v in info.items():
        if not k.startswith(pfx):
            continue
        k = int(k[len(pfx):])
        args.append((k, v))
    args.sort(key=lambda x: x[0])
    return [arg[1] for arg in args]

EXE_SLOTS = ['source', 'args']

class Executable(Target):
    __slots__ = EXE_SLOTS
    target_type = 'executable'

    @classmethod
    def parse(class_, build, mod, external):
        srcname = build.gen_name()
        build.add(
            srcname, source.SourceModule.parse(mod, external=external))
        return class_(
            uuid=mod.info.get_uuid('uuid', None),
            source=srcname,
            filename=mod.info.get_string('filename'),
            args=parse_args(mod.info),
        )

class ApplicationBundle(Target):
    __slots__ = EXE_SLOTS + ['info_plist']
    target_type = 'application_bundle'

    @classmethod
    def parse(class_, build, mod, external):
        srcname = build.gen_name()
        build.add(
            srcname, source.SourceModule.parse(mod, external=external))
        return class_(
            uuid=mod.info.get_uuid('uuid', None),
            source=srcname,
            filename=mod.info.get_string('filename'),
            args=parse_args(mod.info),
            info_plist=mod.info.get_path('info-plist'),
        )
