@echo off
if exist *.wrl del *.wrl
if exist err del err

echo ---
echo Running tests.  This may take a minute...

rem #### Test with ASCII STL files.
stl2vrml.exe testdata\space_invader_1.stl space_invader_1.wrl >> err
stl2vrml.exe testdata\space_invader_2.stl space_invader_2.wrl >> err
stl2vrml.exe testdata\conifer.stl conifer.wrl >> err

rem #### Test with binary STL files.
stl2vrml.exe testdata\doomkeycard.stl doomkeycard.wrl >> err
stl2vrml.exe testdata\grandcanyon.stl grandcanyon.wrl >> err

rem #### Test with very large STL files.
stl2vrml.exe testdata\yowanehaku20130114_002.stl yowanehaku20130114_002.wrl >> err
stl2vrml.exe testdata\CraterLake3.2480_1290_117.stl CraterLake3.2480_1290_117.wrl >> err

rem #### Test intentionally bad STL file.
rem #### This should fail with an error.
echo --- >> err
echo The next test should fail with an error: >> err
echo --- >> err
stl2vrml.exe testdata\badfile.stl badfile.wrl >> err
echo --- >> err

echo Done running tests.
echo Done running tests. >> err

if exist err type err

