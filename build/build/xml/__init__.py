import build.xml.env

def _parse(path, rootenv):
    import build.xml.lxml
    global _parse
    _parse = build.xml.lxml.parse_file
    _parse(path, rootenv)

def parse_file(buildfile, path, vars):
    env = build.xml.env.RootEnv(vars, path.dirname(), buildfile)
    _parse(path.native(), env)
