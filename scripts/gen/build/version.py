from __future__ import with_statement
from gen.path import Path
from cStringIO import StringIO

def gen_version(config):
    vers = config.versions
    fp = StringIO()
    fp.write('/* Generated automatically by build system */\n')
    for k, v in sorted(vers.iteritems()):
        for kk, vv in zip(('VERSION', 'COMMIT'), v):
            fp.write('const char SG_%s_%s[] = "%s";\n' % (k, kk, vv))
    text = fp.getvalue()

    # FIXME this is a hack...
    path = Path(config.repos['SG'], 'sg/core/version_const.c')

    with open(path.native, 'w') as fp:
        fp.write(text)
