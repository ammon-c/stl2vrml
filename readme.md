## stl2vrml - Utility to convert 3D model from STL to VRML (WRL) file format

**Description:**

**Stl2vrml** is a command-line utility that converts a 3D model
from the STL file format to the VRML (WRL) file format.  I find
this useful when I have a 3D print model that's in an STL file
but I want to look at the model in a VRML viewer application.  

**Language:** C++

**Platform:** Windows

**Build:**

Run the build script from the Windows command prompt using
Microsoft's NMAKE utility.  NMAKE is typically installed with
Microsoft Visual Studio.  

**Command Line Usage:**

* stl2vrml *infile*.STL *outfile*.WRL

**Files:**

* **stl2vrml.cpp:** C++ source code for the stl2vrml program.

* **simplefile.h:** C++ class for file handling.

* **makefile:** NMAKE script to build the executable program from the source code.

* **RunTests.bat:** Windows batch script to test stl2vrml by attempting to convert several .STL files from the **testdata** subdirectory into VRML .WRL files.

* **CleanTests.bat:** Windows batch script to remove any test files that were created by **RunTests.bat**.

-*- end -*-
