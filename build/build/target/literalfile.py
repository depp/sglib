from .gensource import GeneratedSource

class LiteralFile(GeneratedSource):
    __slots__ = ['contents']

    def __init__(self, name, target, contents):
        super(LiteralFile, self).__init__(name, target)
        self.contents = contents

    def write(self, fp):
        fp.write(self.contents)
