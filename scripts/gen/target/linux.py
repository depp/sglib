import gen.target.nix as nix

TARGET_DEFAULT = 'make'

def target_make(proj, opts, user_env):
    cfg = nix.NixConfig(opts, user_env, 'linux', proj)
    target = cfg.configure()
    return target
