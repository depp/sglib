__all__ = ['OS', 'INTRINSICS']

# Intrinsics for each OS
OS = {
    'linux': ('linux', 'posix'),
    'osx': ('osx', 'posix'),
    'windows': ('windows',),
}
INTRINSICS = set(x for v in OS.values() for x in v)
