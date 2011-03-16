#!BPY
"""
Name: 'Egg 3D (.egg3d)...'
Blender: 245
Group: 'Export'
Tooltip: 'Export selected mesh to Egg 3D format (.egg3d)'
"""

import Blender
import BPyMesh

import array
import struct
import math

def wrhead(file):
    file.write('Egg3D Model\0')
    file.write(struct.pack('I', 0x01020304))

def pad(file, sz):
    if sz & 3:
        file.write('\0' * (4 - (sz & 3)))

def wrchunk(file, name, data):
    sz = len(data)
    file.write(name)
    file.write(struct.pack('I', sz))
    file.write(data)
    pad(file, sz)

def wrchunka(file, name, arr):
    sz = len(arr) * arr.itemsize
    file.write(name)
    file.write(struct.pack('I', sz))
    arr.tofile(file)
    pad(file, sz)

def wrend(file):
    file.write('END ')

def write(filename):
    start = Blender.sys.time()
    if not filename.lower().endswith('.egg3d'):
        filename += '.egg3d'
    scn = Blender.Scene.GetCurrent()
    ob = scn.objects.active
    if not ob:
        Blender.Draw.PupMenu('Error%t|Select 1 active object')
        return
    mesh = BPyMesh.getMeshFromObject(ob, None, True, False, None)
    if not mesh:
        Blender.Draw.PupMenu('Error%t|Could not get mesh data from active object')
        return
    mesh.transform(ob.matrixWorld)

    file = open(filename, 'wb')
    wrhead(file)

    scale = 0
    for v in mesh.verts:
        scale = max(scale, max(v.co), -min(v.co))
    if scale:
        fac = 32767.0 / scale
        scale = scale / 32767.0
    else:
        fac = 0
    wrchunk(file, 'SclF', struct.pack('f', scale))

    verts = array.array('h')
    for v in mesh.verts:
        verts.extend(int(math.floor(c * fac + 0.5)) for c in v.co)
    wrchunka(file, 'VrtS', verts)

    tris = array.array('H')
    lins = set()
    for f in mesh.faces:
        vs = f.v
        for i in xrange(len(vs) - 2):
            tris.append(vs[0].index)
            tris.append(vs[i+1].index)
            tris.append(vs[i+2].index)
        for i in xrange(len(vs)):
            v0 = vs[i].index
            v1 = vs[i-1].index
            lins.add((min(v0, v1), max(v0, v1)))
    lins2 = array.array('H')
    for l in lins:
        lins2.extend(l)
    lins = lins2
    wrchunka(file, 'LinS', lins)
    wrchunka(file, 'TriS', tris)

    wrend(file)
    file.close()

    end = Blender.sys.time()
    message = 'Successfully exported %s in %.4f seconds' % (repr(Blender.sys.basename(filename)), end - start)
    print message


def main():
    Blender.Window.FileSelector(write, 'Egg 3D Export', Blender.sys.makename(ext='.egg3d'))

if __name__ == '__main__':
    main()
