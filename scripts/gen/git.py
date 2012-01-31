import gen.shell as shell

def describe(obj, path):
    """Call 'git-describe' on the given path."""
    git = obj.opts.git_path or 'git'
    desc = shell.getoutput([git, 'describe'], cwd=path)
    return desc.strip()
