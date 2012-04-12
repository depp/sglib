"""Atoms and atom-specific environments."""
from gen.env import Environment

PLATFORMS = frozenset(['LINUX', 'MACOSX', 'WINDOWS'])

class AtomEnv(object):
    """Class associating atoms with environments."""
    __slots__ = ['_lookup', '_env1', '_envn', '_pre', '_post', '_platform']
    def __init__(self, lookup, pre, post, platform):
        """Construct a new atom environment set.

        The 'lookup' parameter is a sequence of functions.  Each
        function should take a single argument.  Each one is passed
        the atom in turn.  If it returns None, then the next function
        is tried.  If it returns an environment, then no further
        lookup functions are tried.  This class will cache lookups, so
        the lookup function can be arbitrarily expensive.

        The pre and post arguments are put before and after a sequence
        of atom environments.

        The platform is the platform atom.  Any atom list containing a
        platform atom but not the specified platform token will
        generate no environment.
        """
        self._lookup = list(lookup)
        self._pre = pre
        self._post = post
        self._env1 = {}
        self._envn = {}
        self._platform = platform

    def getenv1(self, atom):
        """Get the environment for a specific atom."""
        try:
            return self._env1[atom]
        except KeyError:
            e = None
            for f in self._lookup:
                e = f(atom)
                if e is not None:
                    break
            self._env1[atom] = e
            return e

    def getenv(self, atoms):
        """Get the environment for an atom set.

        This will also include the pre and post environments.  If any
        atom environment is None, then this whole function will return
        None.  If the atom set should not be built for this platform,
        this will return None.
        """
        plats = set()
        libs = set()
        for x in atoms:
            if x in PLATFORMS:
                plats.add(x)
            else:
                libs.add(x)
        if plats and self._platform not in plats:
            return None
        libs = list(sorted(libs))
        lstr = ' '.join(libs)
        try:
            return self._envn[lstr]
        except KeyError:
            es = [self._pre]
            for lib in libs:
                e = self.getenv1(lib)
                if e is None:
                    self._envn[lstr] = None
                    return None
                es.append(e)
            es.append(self._post)
            e = Environment(*es)
            self._envn[lstr] = e
            return e

class SourceEnv(object):
    """Object which lists sources and their environments.
    
    Iterating over this object yields (source, env) pairs, where source
    is a source file and env is the corresponding environment.  This will
    omit any sources which should not be built, according to the
    environment.
    """
    __slots__ = ['_allatoms', '_complete', '_proj', '_atomenv', '_atoms']
    def __init__(self, proj, atomenv, *atoms):
        self._allatoms = set(atoms)
        self._complete = False
        self._proj = proj
        self._atomenv = atomenv
        self._atoms = atoms

    def unionenv(self):
        """Get the combined environment for all sources."""
        if not self._complete:
            for x in self:
                pass
            assert self._complete
        env = self._atomenv.getenv(self._allatoms - PLATFORMS)
        assert env is not None
        return env

    def __iter__(self):
        atoms = self._atoms
        getenv = self._atomenv.getenv
        for source in self._proj.sourcelist.sources():
            env = getenv(source.atoms + atoms)
            if env is not None:
                yield source, env
                self._allatoms.update(source.atoms)
        self._complete = True

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
