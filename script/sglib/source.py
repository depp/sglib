from d3build.source import SourceList

src = SourceList(base=__file__, path='../..')

src.add(path='include/sg', sources='''
arch.h
atomic.h
attribute.h
audio_buffer.h
audio_file.h
binary.h
byteorder.h
clock.h
configfile.h
cpu.h
cvar.h
defs.h
entry.h
error.h
event.h
file.h
hash.h
hashtable.h
key.h
keycode.h
log.h
mixer.h
net.h
opengl.h
pack.h
pixbuf.h
rand.h
record.h
strbuf.h
thread.h
type.h
util.h
version.h
''')

src.add('include/config.h')

src.add(path='src/audio', sources='''
buffer.c
convert.c
file.c
ogg.c ogg
ogg.h ogg
opus.c ogg opus
resample.c
vorbis.c ogg vorbis
wav.c
writer.c
''')

src.add(path='src/core', sources='''
clock.c
clock_impl.h
configfile.c
cvar.c
error.c
file_impl.h
file_posix.c posix
file_read.c
file_win.c windows
keyid.c
keytable_evdev.c linux
keytable_mac.c osx
keytable_win.c windows
log.c
log_console.c
log_impl.h
log_network.c
logtest.c
net.c
opengl.c
pack.c
path.c
path_norm.c
private.h
rand.c
sys.c
version.c
version_const.c
winutf8.c windows
''')

src.add(path='src/mixer', sources='''
channel.c
mixdown.c
mixer.c
mixer.h
queue.c
sound.c
sound.h
time.c
time.h
timeexact.c
system_alsa.c audio_alsa
system_coreaudio.c audio_coreaudio
system_directsound8.c audio_directsound
system_sdl.c audio_sdl
''')

src.add(path='src/pixbuf', sources='''
coregraphics.c image_coregraphics
libjpeg.c image_libjpeg
libpng.c image_libpng
loadimage.c
pixbuf.c
premultiply_alpha.c
private.h
wincodec.cpp image_wincodec
''')

src.add(path='src/record', tags=['video_recording'], sources='''
cmdargs.c
cmdargs.h
record.c !video_recording
record.h !video_recording
screenshot.c !video_recording
screenshot.h !video_recording
videoio.c
videoio.h
videoparam.c
videoparam.h
videoproc.c
videoproc.h
videotime.c
videotime.h
''')

src.add(path='src/type', sources='''
coretext.c type_coretext
pango.c type_pango
uniscribe.c type_uniscribe
''')

src.add(path='src/util', sources='''
cpu.c
hash.c
hashtable.c
strbuf.c
thread_pthread.c posix
thread_windows.c windows
''')

src.add(path='src/core/osx', tags=['frontend_cocoa'], sources='''
GApplication.h
GApplication.m
GController.h
GController.m
GDisplay.h
GDisplay.m
GMain.m
GPrefix.h
GView.h
GView.m
GWindow.h
GWindow.m
''')

src.add(path='src/core/gtk', tags=['frontend_gtk'], sources='''
gtk.c
''')

src.add(path='src/core/sdl', tags=['frontend_sdl'], sources='''
sdl.c
''')

src.add(path='src/core/windows', tags=['frontend_windows'], sources='''
windows.c
''')
