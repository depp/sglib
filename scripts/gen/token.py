class token(object):
    def __init__(self, name):
        self._name = name
    def __repr__(self):
        print '<%s>' % (self._name,)
