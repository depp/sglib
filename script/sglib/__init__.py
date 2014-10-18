from d3build.source import SourceList

class App(object):
    __slots__ = [
        'name', 'source',
        'email', 'uri', 'copyright', 'identifier', 'uuid',
    ]

    def __init__(self, *, name, source,
                 email=None, uri=None, copyright=None,
                 identifier=None, uuid=None):
        self.name = name
        self.source = source
        self.email = email
        self.uri = uri
        self.copyright = copyright
        self.identifier = identifier
        self.uuid = uuid

    def configure(self):
        pass
