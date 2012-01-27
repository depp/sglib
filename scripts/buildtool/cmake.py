import string

TEMPL = """\
# $WARNING
# Only Windows builds supported with CMake
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project($PROJNAME)
if(NOT DEFINED WIN32)
  message(FATAL_ERROR "Only Windows supported with CMake")
endif()
include_directories($INCDIRS)
add_executable($EXENAME $SOURCES)
"""

def run(obj):
    t = string.Template(TEMPL).substitute(
        WARNING=obj._warning,
        PROJNAME='game',
        EXENAME='Game',
        INCDIRS=' '.join(obj._incldirs),
        SOURCES=' '.join(obj._get_atoms(None, 'WINDOWS')))
    obj._write_file('CMakeLists.txt', t)
