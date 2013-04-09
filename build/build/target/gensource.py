HEADER = 'This file automatically generated by the build system.'

class GeneratedSource(object):
    __slots__ = ['name', 'target']

    def __init__(self, name, target):
        self.name = name
        self.target = target

    @property
    def is_target(self):
        return True
