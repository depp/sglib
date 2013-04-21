from . import Target

EXE_SLOTS = ['source', 'filename', 'args']

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

class Executable(Target):
    __slots__ = EXE_SLOTS
    target_type = 'executable'

    @classmethod
    def parse(class_, build, mod):
        source = build.gen_name()
        build.add_sources(source, mod)
        return class_(
            uuid=mod.info.get_uuid('uuid', None),
            source=source,
            filename=mod.info.get_string('filename'),
            args=parse_args(mod.info),
        )

class ApplicationBundle(Target):
    __slots__ = EXE_SLOTS + ['info_plist']
    target_type = 'application_bundle'

    @classmethod
    def parse(class_, build, mod):
        source = build.gen_name()
        build.add_sources(source, mod)
        return class_(
            uuid=mod.info.get_uuid('uuid', None),
            source=source,
            filename=mod.info.get_string('filename'),
            args=parse_args(mod.info),
            info_plist=mod.info.get_path('info-plist'),
        )
