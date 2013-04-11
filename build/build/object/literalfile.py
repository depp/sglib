from . import GeneratedSource

class LiteralFile(GeneratedSource):
    __slots__ = ['contents']

    def write(self, fp, cfg):
        fp.write(self.contents)

    @classmethod
    def parse(class_, build, mod):
        return class_(
            target=mod.info.get_path('target'),
            contents=mod.info.get_string('contents'),
        )
