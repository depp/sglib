import gen.shell as shell

def describe(path):
    """Call 'git-describe' on the given path."""
    desc = shell.getoutput(['git', 'describe'], cwd=path)
    return desc.strip()
