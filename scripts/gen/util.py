import os
import re

# FIXME is this function used?
def relpath(path, base):
    parts = []
    p = path
    r = base + os.path.sep
    while not r.startswith(p + os.path.sep):
        np, c = os.path.split(p)
        if np == p:
            raise ValueError('no common directory: {!r}, {!r}'
                             .format(path, base))
        p = np
        parts.append(c)
    r = base
    while r != p:
        np, c = os.path.split(r)
        assert np != r
        r = np
        parts.append('..')
    parts.reverse()
    return os.path.join(*parts)

_IS_IDENT = re.compile('^[_A-Za-z][_A-Za-z0-9]*$')
def is_ident(x):
    """Get whether the string is a valid identifier."""
    return bool(_IS_IDENT.match(x))

_IS_TITLE = re.compile(r'^\w[-\w]*(?: [-\w]+)*$')
def is_title(x):
    """Get whether the string is a valid title.

    A valid title is more permissive than a filename, it can include
    spaces in addition to alphanumeric characters, hyphens, and
    underscores.  Spaces must be surrounded on both sides by non-space
    characters.  Hyphens may not appear at the beginning.  Titles may
    not be empty.
    """
    return bool(_IS_TITLE.match(x))

_IS_FILENAME = re.compile(r'^[_a-zA-Z0-9][-_a-zA-Z0-9]*$')
def is_filename(x):
    """Get whether the string is a valid filename.

    A valid filename may contain hyphens, underscores, and
    alphanumeric characters.  The string may not be empty and must not
    start with a hyphen.
    """
    return bool(_IS_FILENAME.match(x))

_IS_DOMAIN = re.compile(r'^[a-z0-9](?:[-a-z0-9]*[a-z0-9])?' 
                        r'(?:\.[a-z0-9](?:[-a-z0-9]*[a-z0-9])?)*$')
def is_domain(x):
    """Get whether the string is a plausible domain name."""
    return bool(_IS_DOMAIN.match(x))

_IS_VERSION = re.compile(r'^[-+.A-Za-z0-9]+$')
def is_version(x):
    """Get whether the string is a plausible version number."""
    return bool(_IS_VERSION.match(x))

_IS_HASH = re.compile(r'^[0-9a-f]+$')
def is_hash(x):
    """Get whether the string is a plausible hash."""
    return bool(_IS_HASH.match(x))

_IS_CVAR = re.compile(r'^\w[-\w]*(?:\.[-\w]+)+$')
def is_cvar(x):
    """Get whether the string is a valid CVar name.

    CVars are sequences of parts separated by periods.  There must be
    at least one part.  Each part must be nonempty.  Parts may contain
    alphanumeric characters, underscores, and hyphens.  The first part
    must not start with a hyphen.
    """
    return bool(_IS_CVAR.match(x))

_FILENAME_INVALID_CHAR = re.compile('[^-A-Za-z0-9]+')
def make_filename(x):
    """Change a string into a valid filename.

    This process is rather clumsy and does not always work.  It
    changes every sequence of illegal characters into an underscore.
    """
    y = _FILENAME_INVALID_CHAR.sub('_', x)
    if not is_filename(y):
        raise ValueError('cannot change %s into filename' % (x,))
    return y
