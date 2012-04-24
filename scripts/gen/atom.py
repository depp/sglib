"""Project- and target-specific environments.

Atoms are simple strings attached to source files to describe when and
how they are built.  There are four kinds of atoms.

* Platform atoms (LINUX, MACOSX, WINDOWS) specify that the source file
  should only be built on that platform.  If multiple platform atoms
  are specified, then the file will be built on all of the listed
  platforms.

* Module atoms (SGLIB, SGLIBXX) specify that the source file should
  only be built when that module is being built.  If multiple module
  atoms are specified, then the file will only be built when all of
  those modules are being built.  If no module atoms are specified,
  the file will always be built (but this may change).  When a source
  file is built, the environments from all of its modules and
  dependent modules are added to the build environment.

* Library atoms (LIBPNG, LIBJPEG, PANGO) specify that the source file
  requires the presence of the given library in order to function.
  The build will fail if a required library is missing.  When a source
  file is built, the environment of all libraries it requires are
  added to the build environment.

* Architecture atoms (ppc, x86, x64) specify that the source file
  should only be built on that architecture.  The build environment
  for an architecture is applied to all files on that architecture,
  regardless of whether they are marked with the architecture's atom.
  This is used to perform cross-compilation on platforms that support
  it (Mac OS X, Windows).
"""
from gen.env import Environment
import gen.info as info

PLATFORMS = frozenset(['LINUX', 'MACOSX', 'WINDOWS'])
ARCHS = frozenset(['ppc', 'ppc64', 'x86', 'x64'])

class MissingLibrary(Exception):
    """Exception for missing libraries."""
    __slots__ = ['_atom']

    def __init__(self, atom):
        self._atom = atom

    def __repr__(self):
        return 'MissingLibrary(%r)' % (self._atom,)

    def __str__(self):
        return 'missing library: %s' % (self._atom,)

class UnknownArchitecture(Exception):
    """Exception for missing architectures."""
    __slots__ = ['_atom']

    def __init__(self, atom):
        self._atom = atom

    def __repr__(self):
        return 'UnknownArchitecture(%r)' % (self._atom,)

    def __str__(self):
        return 'unknown architecture: %s' % (self._atom,)

def lookup_dummy(obj):
    return None

class ProjectEnv(object):
    """Object describing project-wide environments."""
    __slots__ = ['_proj', '_lookup', '_env', '_lib_cache', '_mod_cache',
                 '_multi_cache', '_libs', '_modules', '_mod_reqs']

    def __init__(self, proj, lookup=None, env=None):
        """Construct a new project environment set.

        The 'proj' parameter is the Project.  The project is used for
        identifying which atoms correspond to modules.

        The 'lookup' parameter is a function which takes a single atom
        as an argument: architecture, library, or module (but not
        platform).  For libraries and architectures, if None is
        returned, that means that the library or architecture is
        missing.  For modules, if None is returned then that means
        that the module has no special flags.

        This class will cache lookups, so the lookup function can be
        arbitrarily expensive (e.g., it can run pkg-config).

        The 'env' parameter is an environment which is applied to all
        source files.
        """
        if lookup is None:
            lookup = lookup_dummy
        mreqs = {}
        for m in proj.modules:
            x = set([m])
            y = x
            while y:
                z = set()
                for i in y:
                    z.update(proj.modules[i].reqs)
                y = z - x
                x.update(z)
            mreqs[m] = frozenset(x)
        self._proj = proj
        self._lookup = lookup
        self._env = env
        self._lib_cache = {}
        self._mod_cache = {}
        self._multi_cache = {}
        self._modules = frozenset(proj.modules)
        self._mod_reqs = mreqs

    @property
    def project(self):
        return self._proj

    def module_closure(self, atoms):
        """Get the dependencies for a given list of modules.

        Returns a new set.
        """
        x = set()
        for a in atoms:
            x.update(self._mod_reqs[a])
        return x

    def module_reqs(self, atom):
        """Get the module dependencies for a given module."""
        return self._mod_reqs[atom]

    def module_env(self, atom):
        """Get the environment for a specific module."""
        try:
            return self._mod_cache[atom]
        except KeyError:
            m = self._proj.modules[atom]
            re = [self.module_env(r) for r in m.reqs]
            e = Environment(CPPPATH=m.ipath, DEFS=m.defs, *re)
            self._mod_cache[atom] = e
            return e

    def atom_env(self, atom):
        """Get the environment for a specific atom.

        Returns None if the environment is missing.
        """
        try:
            return self._lib_cache[atom]
        except KeyError:
            if atom == 'EXTERNAL':
                e = Environment(_EXTERNAL=True)
            else:
                e = self._lookup(atom)
            self._lib_cache[atom] = e
            return e

    def multi_env(self, modules, libs, arch):
        """Get the environment for the given libraries and modules.

        The will raise a MissingLibrary exception if any library is
        missing, and raise UnknownArchitecture if the architecture has
        no environment.
        """
        modules = list(sorted(set(modules)))
        libs = list(sorted(set(libs)))
        k = ' '.join(modules + libs)
        if arch is not None:
            k += ' ' + arch
        try:
            return self._multi_cache[k]
        except KeyError:
            pass
        elist = []
        for a in modules:
            elist.append(self.module_env(a))
            e = self.atom_env(a)
            if e is not None:
                elist.append(e)
        for a in libs:
            e = self.atom_env(a)
            if e is None:
                raise MissingLibrary(a)
            elist.append(e)
        if arch is not None:
            e = self.atom_env(arch)
            if e is None:
                raise UnknownArchitecture(arch)
            elist.append(e)
        if self._env is not None:
            elist.append(self._env)
        e = Environment(*elist)
        self._multi_cache[k] = e
        return e

    def targets(self, platform):
        """Get the target environment for every target.

        Iterates over TargetEnv objects.
        """
        for target in self.project.targets():
            yield TargetEnv(self, target.atom, platform)

class TargetEnv(object):
    """Object describing a target-specific environment.

    Iterating over this object yields (source, env) pairs, where source
    is a source file and env is the corresponding environment.  This will
    omit any sources which should not be built, according to the
    environment and the target being built.

    Any property which is a key for the project info or key for the
    target info can be accessed through this object.

    This object is stateful!  It has an architecture which can be set.
    """
    __slots__ = ['_projenv', '_platform', '_module',
                 '_modules', '_arch']

    def __init__(self, projenv, module, platform):
        self._projenv = projenv
        self._platform = platform
        self._module = module
        self._modules = projenv.module_closure([module])
        self._arch = None

    def setarch(self, arch):
        if arch is not None and arch not in ARCHS:
            raise ValueError('unknown architecture: %s' % (arch,))
        self._arch = arch

    def getarch(self):
        return self._arch

    arch = property(getarch, setarch)
    del getarch
    del setarch

    @property
    def default_cvars(self):
        """Get the default cvars for this target."""
        pcvars = self.project.info.DEFAULT_CVARS
        mcvars = self.main.info.DEFAULT_CVARS
        return gen.info.CVars.combine(pcvars, mcvars)

    @property
    def project(self):
        """Get the project for this target."""
        return self._projenv.project

    def modules(self):
        """Get all modules included by this target."""
        mm = self.project.modules
        for m in self._modules:
            return mm[m]

    @property
    def main(self):
        """Get the main module for this target."""
        return self.project.modules[self._module]

    @property
    def simple_name(self):
        """Get a simple name for this target.

        This is usable for constructing intermediate object paths.
        """
        return self._module.lower()

    def _iter_atoms(self):
        """Iterate yielding (source, modules, libs)."""
        arch = self._arch
        mymod = self._modules
        allmod = self._projenv._modules
        allplat = PLATFORMS
        allarch = ARCHS
        nonlib = allmod.union(allplat).union(allarch)
        plat = self._platform
        for source in self._projenv._proj.sourcelist.sources():
            atoms = set(source.atoms)
            mset = self._projenv.module_closure(atoms & allmod)
            if not mset.issubset(mymod):
                continue
            aset = atoms & allarch
            if aset and arch not in aset:
                continue
            pset = atoms & allplat
            if pset and plat not in pset:
                continue
            lset = atoms - nonlib
            yield source, mset, lset

    def unionenv(self):
        """Get the combined environment for all sources."""
        all_libs = set()
        if self._arch is not None:
            all_libs.add(self._arch)
        for source, mset, lset in self._iter_atoms():
            all_libs.update(lset)
        return self._projenv.multi_env(self._modules, all_libs, self._arch)

    def __iter__(self):
        arch = self._arch
        for source, mset, lset in self._iter_atoms():
            env = self._projenv.multi_env(mset, lset, arch)
            yield source, env

    def apply(self, handlers):
        """Apply handlers to each source file.
        
        The handlers are chosen by looking up the source file types
        in the handlers dictionary.  Each handler should either be
        None, or a callable that takes a source and an environment
        for that source.  If the handler is None, sources of that type
        will be ignored.

        If there is no handler for a given source file, this will
        raise an exception.
        """
        for source, env in self:
            stype = source.sourcetype
            try:
                h = handlers[stype]
            except KeyError:
                raise Exception(
                    'cannot handle file type %s for path %s' %
                    (stype, source.relpath.posix))
            if h is not None:
                h(source, env)

    def types(self):
        """Get a set of all source types in the target."""
        types = set()
        for source, mset, lset in self._iter_atoms():
            types.add(source.sourcetype)
        return types

    def __getattr__(self, key):
        mk = info.ExecInfo.getkey(key)
        pk = info.ProjectInfo.getkey(key)
        if mk is None:
            if pk is None:
                raise AttributeError(key)
            else:
                return pk.__get__(self.project.info, None)
        else:
            if pk is None:
                return mk.__get__(self.main.info, None)
            else:
                assert mk.__class__ is pk.__class__
                pv = pk.__get__(self.project.info, None)
                mv = mk.__get__(self.main.info, None)
                return pk.combine(pv, mv)
