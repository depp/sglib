from __future__ import with_statement

from gen.path import Path
import gen.project
import os
import sys

CACHE_FILE = 'init.cache.dat'

def parse_feature_args(proj, keys, p):
    import optparse
    ge = optparse.OptionGroup(p, 'Optional Features')
    gw = optparse.OptionGroup(p, 'Optional Packages')
    p.add_option_group(ge)
    p.add_option_group(gw)

    optlibs = set()
    for f in proj.feature:
        default = True
        nm = f.modid.lower()
        desc = '%s feature' % f.modid if f.desc is None else f.desc
        var = 'enable_%s' % f.modid
        keys.append(var)
        ge.add_option(
            '--enable-%s' % nm,
            default=default,
            action='store_true',
            dest=var,
            help=('enable %s' % desc if not default
                  else optparse.SUPPRESS_HELP))
        ge.add_option(
            '--disable-%s' % nm,
            default=default,
            action='store_false',
            dest=var,
            help=('disable %s' % desc if default
                  else optparse.SUPPRESS_HELP))

        for i in f.impl:
            optlibs.update(i.require)

    del f

    for m in proj.modules:
        if not isinstance(m, gen.project.ExternalLibrary):
            continue
        nm = m.modid.lower()
        desc = m.name if m.name is not None else '%s library' % m.modid
        has_bundled = False
        for s in m.sources:
            if isinstance(s, gen.project.BundledLibrary):
                has_bundled = True
        opts = []
        if m.modid in optlibs:
            opts.append((nm, 'with_%s' % m.modid, desc))
        if has_bundled:
            opts.append(('bundled-%s' % nm, 'bundled_%s' % m.modid,
                         '%s (bundled copy)' % desc))

        for argn, var, desc in opts:
            keys.append(var)
            gw.add_option(
                '--with-%s' % argn,
                default=None,
                action='store_true',
                dest=var,
                help='use %s' % desc)
            gw.add_option(
                '--without-%s' % argn,
                default=None,
                action='store_false',
                dest=var,
                help=optparse.SUPPRESS_HELP)

class Configuration(object):
    __slots__ = ['argv', 'project', 'opts', 'vars']

    def __init__(self):
        self.argv = None
        self.project = None
        self.opts = None
        self.vars = None

    def reconfig(self):
        import optparse
        import gen.xml as xml

        proj = xml.load('project.xml', Path())
        for modpath in proj.module_path:
            for fname in os.listdir(modpath.native):
                if fname.startswith('.') or not fname.endswith('.xml'):
                    continue
                mod = xml.load(os.path.join(modpath.native, fname), modpath)
                proj.add_module(mod)
        self.project = proj

        p = optparse.OptionParser()
        keys = []
        parse_feature_args(proj, keys, p)

        opts, args = p.parse_args(self.argv)
        opt_dict = {}
        for k in keys:
            opt_dict[k] = getattr(opts, k)

        var_dict = {}
        targets = []
        for arg in args:
            i = arg.find('=')
            if i >= 0:
                var_dict[arg[:i]] = arg[i+1:]
            else:
                targets.append(arg)

        self.opts = opts
        self.vars = var_dict
        if not targets:
            targets = ['default']
        return targets

    def build(self, targets):
        pass

    def store(self):
        import cPickle
        try:
            os.unlink(CACHE_FILE)
        except OSError:
            pass
        try:
            with open(CACHE_FILE, 'wb') as fp:
                cPickle.dump(self, fp, cPickle.HIGHEST_PROTOCOL)
        except:
            try:
                os.unlink(CACHE_FILE)
            except OSError:
                pass

def load():
    import cPickle
    try:
        fp = open(CACHE_FILE, 'rb')
        obj = cPickle.load(fp)
        fp.close()
        return obj
    except (OSError, cPickle.PickleError):
        sys.stderr.write(
            'error: could not load configuration cache\n'
            'please run init.sh again\n')
        sys.exit(1)

def run():
    mode = sys.argv[1]
    if mode == 'config':
        cfg = Configuration()
        cfg.argv = sys.argv[2:]
        targets = cfg.reconfig()
        cfg.store()
        cfg.build(targets)
    elif mode == 'reconfig':
        cfg = load()
        cfg.reconfig()
        cfg.store()
    elif mode == 'build':
        cfg = load()
        cfg.build(sys.argv[2:])
    else:
        sys.stderr.write('error: invalid mode: %s\n' % mode)
        sys.exit(1)
