from .gensource import GeneratedSource

class TemplateFile(GeneratedSource):
    __slots__ = ['source', 'tvars']

    def __init__(self, name, target, source, tvars):
        super(TemplateFile, self).__init__(name, target)
        self.source = source
        self.tvars = tvars

    @property
    def deps(self):
        return (self.source,)

    def write(self, fp):
        from build import template
        import sys
        print('warning: this is a hack', file=sys.stderr)
        with open(self.source.to_posix()) as infp:
            srcdata = infp.read()
        fp.write(template.expand(srcdata, self.tvars))
