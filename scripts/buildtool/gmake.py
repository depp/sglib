import os
import buildtool.path as path
import re

def objs(paths):
    return ' '.join(path.withext(path.sources(paths), '.o'))

def run(obj):
    base_src = obj._get_atoms(None, 'LINUX')
    gtk_src = obj._get_atoms('GTK')
    sdl_src = obj._get_atoms('SDL')
    libpng_src = obj._get_atoms('LIBPNG')
    libjpeg_src = obj._get_atoms('LIBJPEG')
    pango_src = obj._get_atoms('PANGO')

    all_src = (base_src + gtk_src + sdl_src +
               libpng_src + libjpeg_src + pango_src)

    subs = {
        'BASE_OBJS': objs(base_src),
        'GTK_OBJS': objs(gtk_src),
        'SDL_OBJS': objs(sdl_src),
        'LIBPNG_OBJS': objs(libpng_src),
        'LIBJPEG_OBJS': objs(libjpeg_src),
        'PANGO_OBJS': objs(pango_src),
        'C_OBJS': objs(path.c_sources(all_src)),
        'CXX_OBJS': objs(path.cxx_sources(all_src)),
    }

    def repl(m):
        what = m.group(1)
        try:
            return subs[what]
        except KeyError:
            return m.group(0)

    inpath = os.path.join(os.path.dirname(__file__), 'gmake.txt')
    text = open(inpath, 'r').read()
    text = ('# ' + obj._warning + '\n' +
            re.sub(r'@(\w+)@', repl, text))
    obj._write_file('src/Makefile', text)
