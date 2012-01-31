import gen.shell as shell
import subprocess

def describe(obj, path):
    """Call 'git-describe' on the given, or fall back to SHA-1."""
    git = obj.opts.git_path or 'git'
    try:
        desc = shell.getoutput([git, 'describe'], cwd=path)
    except subprocess.CalledProcessError:
        desc = shell.getoutput([git, 'rev-parse', 'HEAD'], cwd=path)
    return desc.strip()
