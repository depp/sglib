import collections

Project = collections.namedtuple(
    'Project', 'targets files')

Executable = collections.namedtuple(
    'Executable', 'filename exe_icon apple_category cvars envs sources')

Source = collections.namedtuple(
    'Source', 'path envs generator type')
