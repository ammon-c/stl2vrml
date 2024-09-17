#-----------------------------------------------------------------------
# Makefile to build the STL2VRML utility from the C++ source code.
# Requires Microsoft Visual Studio with C++ compiler and NMAKE utility.
# To build, run "NMAKE" from the Windows command prompt.
#-----------------------------------------------------------------------

#
# Compiler options
#
CDEFS=-DWIN32 -D_UNICODE -DUNICODE 
CFLAGS=-c -nologo -Gs -EHsc -Od -W4 -WX -MTd -Zi -I. -Zc:wchar_t $(CDEFS)

#
# Linker options
#
LFLAGS=/NOLOGO /DEBUG gdi32.lib user32.lib kernel32.lib advapi32.lib

#
# Inference rules
#

.SUFFIXES:
.SUFFIXES:   .cpp

.cpp.obj:
   cl $(CFLAGS) $<

#
# Targets
#

all:  stl2vrml.exe

stl2vrml.exe:   stl2vrml.obj
   link /OUT:$@ $(LFLAGS) $**

stl2vrml.obj:   stl2vrml.cpp simplefile.h

# Prepare for fresh build.
# On command line use "NMAKE clean".

clean:
    if exist *.exe del *.exe
    if exist *.obj del *.obj
    if exist *.pdb del *.pdb
    if exist *.bak del *.bak
    if exist *.ilk del *.ilk
    if exist *.out del *.out
    if exist *.wrl del *.wrl
    if exist err del err

