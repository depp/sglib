import os

C_EXTS = ['.c']
CXX_EXTS = ['.cpp', '.cxx', '.cp']
H_EXTS = ['.h']
HXX_EXTS = ['.hpp', '.hxx']
OBJC_EXTS = ['.m']
OBJCXX_EXTS = ['.mm']

TYPES = {'c': C_EXTS, 'h': H_EXTS, 'cxx': CXX_EXTS,
         'hxx': HXX_EXTS, 'm': OBJC_EXTS, 'mm': OBJCXX_EXTS}
def _compute_exts():
    exts = {}
    for what, texts in TYPES.iteritems():
        for ext in texts:
            exts[ext] = what
    return exts
EXTS = _compute_exts()

def filterexts(paths, exts):
    """Return a list of the paths whose extensions are in exts."""
    return [x for x in paths if os.path.splitext(x)[1] in exts]

def c_sources(paths):
    """Return a list of the paths which are C sources."""
    return filterexts(paths, C_EXTS)

def cxx_sources(paths):
    """Return a list of the paths which are C++ sources."""
    return filterexts(paths, CXX_EXTS)

def sources(paths):
    """Return a list of the paths which are sources (not headers)."""
    return filterexts(paths, C_EXTS + CXX_EXTS)

def withext(paths, ext):
    """Return paths with their extensions changed to ext."""
    return [os.path.splitext(x)[0] + ext for x in paths]
