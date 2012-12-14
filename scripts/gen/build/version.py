from gen.path import Path
from io import StringIO

def gen_version(config):
    vers = config.versions
    fp = StringIO()
    fp.write('/* Generated automatically by build system */\n')
    for k, v in sorted(vers.items()):
        for kk, vv in zip(('VERSION', 'COMMIT'), v):
            fp.write('const char SG_{}_{}[] = "{}";\n'.format(k, kk, vv))
    text = fp.getvalue()

    path = config.actions['version'][0]

    with open(path.native, 'w') as fp:
        fp.write(text)
