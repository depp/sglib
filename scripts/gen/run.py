import optparse
import gen.xml as xml
from gen.path import Path
import os

def run():
    proj = xml.load('project.xml', Path())

    for modpath in proj.module_path:
        for fname in os.listdir(modpath.native):
            if fname.startswith('.') or not fname.endswith('.xml'):
                continue
            mod = xml.load(os.path.join(modpath.native, fname), modpath)
            proj.add_module(mod)

    print proj.modules
