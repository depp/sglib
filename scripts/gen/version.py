import re

_VERSION_SEGMENT = re.compile(r'([^0-9]*)([0-9]*)')

def version_cmp(x, y):
    """Compare two strings as version numbers."""
    px = 0
    py = 0
    while True:
        mx = _VERSION_SEGMENT.match(x, px)
        my = _VERSION_SEGMENT.match(y, py)
        ax, bx = mx.groups()
        ay, by = my.groups()
        c = cmp(ax, ay)
        if c:
            return c
        bx = int(bx) if bx else 0
        by = int(by) if by else 0
        c = cmp(bx, by)
        if c:
            return c
        if not ax and not bx:
            return 0
        px = mx.end()
        py = my.end()

if __name__ == '__main__':
    import sys
    TESTS = [
        ('1.10', '1.11', -1),
        ('1.10', '1.9', 1),
        ('weasel-59.7', 'weasel-59.10', -1),
        ('weasel-59.7', 'weasea-59.7', 1),
        ('9.8', '9.8', 0),
        ('9.99', '9.98.1', 1),
    ]
    def rel(c):
        if c < 0: return '<'
        if c > 0: return '>'
        return '='
    for x, y, c in TESTS:
        cc = version_cmp(x, y)
        if cc != c:
            print 'expected %s %s %s; got %s %s %s' % (
                x, rel(c), y, x, rel(cc), y)
            sys.exit(1)
