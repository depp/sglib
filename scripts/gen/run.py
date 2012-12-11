import optparse
import gen.xml as xml
from gen.path import Path
import gen.project
import os

def parse_feature_args(proj, p):
    ge = optparse.OptionGroup(p, 'Optional Features')
    gw = optparse.OptionGroup(p, 'Optional Packages')
    p.add_option_group(ge)
    p.add_option_group(gw)

    optlibs = set()
    for f in proj.feature:
        default = True
        nm = f.modid.lower()
        desc = '%s feature' % f.modid if f.desc is None else f.desc
        ge.add_option(
            '--enable-%s' % nm,
            default=default,
            action='store_true',
            dest='enable_%s' % f.modid,
            help=('enable %s' % desc if not default
                  else optparse.SUPPRESS_HELP))
        ge.add_option(
            '--disable-%s' % nm,
            default=default,
            action='store_false',
            dest='enable_%s' % f.modid,
            help=('disable %s' % desc if default
                  else optparse.SUPPRESS_HELP))

        for i in f.impl:
            optlibs.update(i.require)

    optlibs.difference_update(x.upper() for x in gen.project.OS)
    for libid in optlibs:
        try:
            m = proj.module_names[libid]
        except KeyError:
            sys.stderr.write('warning: cannod find module %s' % (libid))
            continue
        nm = m.modid.lower()
        desc = m.name if m.name is not None else '%s library' % m.modid
        gw.add_option(
            '--with-%s' % nm,
            default=None,
            action='store_true',
            dest='with_%s' % f.modid,
            help='use %s' % desc)
        gw.add_option(
            '--without-%s' % nm,
            default=None,
            action='store_false',
            dest='with_%s' % f.modid,
            help=optparse.SUPPRESS_HELP)

def run():
    proj = xml.load('project.xml', Path())

    for modpath in proj.module_path:
        for fname in os.listdir(modpath.native):
            if fname.startswith('.') or not fname.endswith('.xml'):
                continue
            mod = xml.load(os.path.join(modpath.native, fname), modpath)
            proj.add_module(mod)

    p = optparse.OptionParser()
    parse_feature_args(proj, p)

    opts, args = p.parse_args()
