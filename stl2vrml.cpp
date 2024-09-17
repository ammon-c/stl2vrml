//--------------------------------------------------------------------
// stl2vrml.cpp
// A program to convert a 3D model from .STL file format to .WRL
// (VRML) file format, e.g. for 3D printing.  Supports both the ASCII
// and binary forms of the .STL file format.
//
// (C) Copyright 2018 Ammon R. Campbell.
//
// I wrote this code for use in my own educational and experimental
// programs, but you may also freely use it in yours as long as you
// abide by the following terms and conditions:
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above
//     copyright notice, this list of conditions and the following
//     disclaimer in the documentation and/or other materials
//     provided with the distribution.
//   * The name(s) of the author(s) and contributors (if any) may not
//     be used to endorse or promote products derived from this
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.  IN OTHER WORDS, USE AT YOUR OWN RISK, NOT OURS.  
//--------------------------------------------------------------------
//
// Usage:
//
// Run the program from the command line with two filename arguments:
//
//    stl2vrml infile.stl outfile.wrl
//
// The 3D model is read from the first file (in .STL format) and
// written to the second file (in .WRL format).
//
//--------------------------------------------------------------------
//
// Limitations / Bugs:
//
// * Doesn't work with binary .STL files larger than 2GB.  No size
//   limit for ASCII .STL files.
// * Parse errors don't include any information in the error message
//   about where in the STL file the error happened.
//
//--------------------------------------------------------------------
//
// Reference Material:
//
//  * https://en.wikipedia.org/wiki/STL_(file_format)
//  * https://tecfa.unige.ch/guides/vrml/vrmlman/node6.html
//  * http://www2.cmp.uea.ac.uk/~jrk/wwwvrml.dir/public-vrml/VRMLLECTURE/
//
//--------------------------------------------------------------------

#include "SimpleFile.h"
#include <vector>
#include <algorithm>
#include <assert.h>
#include <ctype.h>

//--------------------------------------------------------------------
// Simple container for a 3D coordinate.
//--------------------------------------------------------------------
struct Point
{
   double x = 0., y = 0., z = 0.;
};

//--------------------------------------------------------------------
// Function to split a string into fields delimited by spaces, commas,
// and/or semicolons.
//--------------------------------------------------------------------
std::vector<std::string> StringFields(const std::string &str)
{
   std::vector<std::string> fields;
   const char *pstr = str.c_str();
   while (*pstr)
   {
      std::string field;
      while (isspace(*pstr))              ++pstr;
      if (*pstr == ',' || *pstr == ';')   ++pstr;
      while (isspace(*pstr))              ++pstr;
      while (*pstr && !isspace(*pstr) && *pstr != ',' && *pstr != ';')
         field += *pstr++;
      fields.push_back(field);
   }

   return fields;
}

//--------------------------------------------------------------------
// VrmlWriter:  This class may be used to write a simple 3D model,
// consisting of triangles, to a VRML .WRL file.  The member functions
// of this class generally throw a string in the event of an error.
//--------------------------------------------------------------------
class VrmlWriter
{
public:
   VrmlWriter() = delete;
   VrmlWriter(const VrmlWriter &) = delete;
   explicit VrmlWriter(File &file) : m_file(file) { }

   //--------------------------------------------------------------------
   // Writes the beginning portion of the WRL file.
   //--------------------------------------------------------------------
   void WriteStartOfWrl()
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      if (!m_file.Printf("#VRML V2.0 utf8\r\n# Model converted by stl2vrml.\r\n"))
         throw writeError;
   }

   //--------------------------------------------------------------------
   // Writes a 3D triangle to the WRL file.
   // Since individual facets are inefficient in WRL file format, we don't
   // write the triangle immediately.  Instead, we accumulate the triangle
   // into a buffer, and the buffer is written to the WRL file as a face
   // set object once it contains a sufficiently large number of
   // triangles.
   //--------------------------------------------------------------------
   void WriteFacetToWrl(const std::vector<Point> &coords)
   {
      // Save facet to list of facets.
      for (const auto &pt : coords)
         m_triangles.push_back(pt);

      // Write the list of facets when it gets big enough.
      constexpr size_t maxPointsPerFaceSet = 3000;
      if (m_triangles.size() >= maxPointsPerFaceSet)
      {
         WriteFacetsToWrl(m_triangles);
         m_triangles.clear();
      }
   }

   //--------------------------------------------------------------------
   // Writes the remainder of the WRL file after all of the facets have
   // been given to us.  The minimum and maximum bounds of the 3D model
   // should be provided in emin and emax.
   //--------------------------------------------------------------------
   void WriteEndOfWrl(const Point &emin, const Point &emax)
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());

      // Write any remaining facets that haven't been written yet.
      if (!m_triangles.empty())
         WriteFacetsToWrl(m_triangles);

      // Point the camera at the model.
      if (!m_file.Printf("\r\nViewpoint {\r\n"
                    "  description \"View_1\"\r\n"
                    "  orientation 1 0 0 0\r\n"))
         throw writeError;
      if (!m_file.Printf("  position %G %G %G\n",
            emin.x + (emax.x - emin.x) / 2.,
            emin.y + (emax.y - emin.y) / 2.,
            emin.z + (emax.z - emin.z) / 2 + std::max(emax.x - emin.x, emax.y - emin.y)))
         throw writeError;
      if (!m_file.Printf("}\r\n"))
         throw writeError;
 
      // Set a dark gray background.
      if (!m_file.Printf("Background { skyColor 0.4 0.4 0.4 }\r\n"))
         throw writeError;
   
      // Default to "examine" mode when the VRML viewer opens the WRL file.
      if (!m_file.Printf("NavigationInfo { type [ \"EXAMINE\" \"ANY\" ] }\r\n\r\n"))
         throw writeError;
   }

private:

   //--------------------------------------------------------------------
   // Writes a list of coordinates values for an IndexedFaceSet in a WRL.
   //--------------------------------------------------------------------
   void WriteCoordListToWrl(const std::vector<Point> &points)
   {
      // Each line contains one XYZ coordinate with spaces
      // between the fields.
      // All but the last line ends with a comma.
      size_t pointsWritten = 0;
      for (const auto &point : points)
      {
         if (!m_file.Printf("        "))
            throw writeError;

         if (!m_file.Printf("%.15G %.15G %.15G", point.x, point.y, point.z))
            throw writeError;

         if (pointsWritten < points.size() - 1)
            if (!m_file.Printf(", "))
               throw writeError;

         if (!m_file.Printf("\r\n"))
            throw writeError;

         ++pointsWritten;
      }
   }

   //--------------------------------------------------------------------
   // Writes a list of coordinate indexes for an IndexedFaceSet in a WRL.
   //--------------------------------------------------------------------
   void WriteCoordIndexesToWrl(const size_t numTriangles)
   {
      // Each line contains the three coordinate indexes for one triangle
      // facet.  The indexes refer to points in the previously written
      // coordinate list.  The format of each line of coorindate indexes
      // is:
      //        index1 index2 index3 -1
      //
      // All but the last line should also have a comma at the end.
      for (size_t trndx = 0; trndx < numTriangles; ++trndx)
      {
         if (!m_file.Printf("      "))
            throw writeError;

         for (size_t i = 0; i < 3; ++i)
            if (!m_file.Printf("%zu, ", trndx * 3 + i))
               throw writeError;

         m_file.Printf("-1");
         if (trndx < numTriangles - 1)
            if (!m_file.Printf(","))
               throw writeError;

         if (!m_file.Printf("\r\n"))
            throw writeError;
      }
   }

   //--------------------------------------------------------------------
   // Writes a collection of 3D triangles to the WRL file as a Shape
   // object with an IndexedFaceSet inside.
   //--------------------------------------------------------------------
   void WriteFacetsToWrl(const std::vector<Point> &points)
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      assert((points.size() % 3) == 0);

      const size_t numTriangles = points.size() / 3;
      if (numTriangles < 1)
         return;  // Nothing to write.

      // We don't have any color information about this model,
      // so we're using a light gray color for the triangles
      // in the WRL file.

      if (!m_file.Printf("\r\nShape {\r\n"
                         "  appearance Appearance {\r\n"
                         "    material Material {\r\n"
                         "      diffuseColor 0.8 0.8 0.8\r\n"
                         "    }\r\n"
                         "  }\r\n"
                         "  geometry IndexedFaceSet {\r\n"
                         "    coord Coordinate {\r\n"
                         "      point [\r\n"))
         throw writeError;

      WriteCoordListToWrl(points);

      if (!m_file.Printf("      ]\r\n"
                         "    }\r\n"
                         "    coordIndex [\r\n"))
         throw writeError;

      WriteCoordIndexesToWrl(numTriangles);

      if (!m_file.Printf("    ]\r\n"
                         "  }\r\n"
                         "}\r\n"))
         throw writeError;
   }

private:
   // The file we're writing.
   File &m_file;

   // Triangles that haven't been written to the file yet.
   // We accumulate them here and write them in batches,
   // rather than writing them one-by-one.
   std::vector<Point> m_triangles;

   // Common error message string used by member functions above.
   const wchar_t *writeError = L"Failed writing to file.";
};

//--------------------------------------------------------------------
// StlReader:  This class may be used to read a 3D model, consisting
// of triangles, from a .STL file.  Supports both ASCII and binary
// .STL files  The member functions of this class generally throw a
// string in the event of an error.
//
// TODO: Consider splitting this into two classes, BinaryStlReader
//       and AsciiStlReader.
//--------------------------------------------------------------------
class StlReader
{
public:
   StlReader() = delete;
   StlReader(const StlReader &) = delete;
   explicit StlReader(File &file) : m_file(file) { }

   //--------------------------------------------------------------------
   // Reads and validates the header of the STL file.
   // Throws if error or if file isn't an STL file.
   //--------------------------------------------------------------------
   void ReadHeaderFromStl()
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      m_file.Seek(0);
      m_numFacets = m_curFacet = 0;
      m_isBinaryStl = IsBinaryStl(m_file);
      m_isBinaryStl ? ReadHeaderFromBinaryStl(m_numFacets) : ReadHeaderFromAsciiStl();
   }

   //--------------------------------------------------------------------
   // Reads the next facet (triangle) from the STL model.
   // Returns false if there are no more facets in the file.
   // The facet's vertex coordinates are returned in "coords".
   //--------------------------------------------------------------------
   bool ReadFacetFromStl(std::vector<Point> &coords)
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      coords.clear();
      return m_isBinaryStl ? ReadFacetFromBinaryStl(coords) : ReadFacetFromAsciiStl(coords);
   }

   //--------------------------------------------------------------------
   // Clean up after reading an STL file.
   //--------------------------------------------------------------------
   void Close()
   {
      m_file.Close();
   }

private:

   //--------------------------------------------------------------------
   // A binary STL record as it appears in the file.
   //--------------------------------------------------------------------
   struct BinaryStlRecord
   {
   #pragma pack(push)
   #pragma pack(1)
   
      float m_normal[3] = {0};            // Surface normal of triangle.
      float m_coorddata[9] = {0};         // Corner points of the triangle.
      std::uint16_t m_attributeSize = 0;  // Reserved.  Should always be zero.
   
   #pragma pack(pop)
   };

   //--------------------------------------------------------------------
   // Reads the header portion of a binary STL file.
   // The facet count from the header is placed into numFacets.
   // Errors throw.
   //--------------------------------------------------------------------
   void ReadHeaderFromBinaryStl(size_t &numFacets)
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      numFacets = 0;

      // Skip the 80 byte comment header at the beginning of the binary STL.
      if (!m_file.Seek(80))
         throw "Seek error reading header of binary STL file.";
   
      // Retrieve the facet count.
      std::uint32_t facets = 0;
      if (m_file.Read(&facets, sizeof(facets)) != sizeof(facets))
         throw "Failed reading facet count from binary STL file.";
      if (facets < 1)
         throw "Facet count in file header is not valid.";

      numFacets = static_cast<size_t>(facets);
   }

   //--------------------------------------------------------------------
   // Reads the header portion of an ASCII STL file.
   // Errors throw.
   //--------------------------------------------------------------------
   void ReadHeaderFromAsciiStl()
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      m_file.Seek(0);
      static char *badStl = "File does not appear to be in STL format.\r\n";

      std::string text;
      if (!m_file.ReadLine(text, true))
         throw badStl;

      // Split the input line into fields/tokens.
      auto fields = StringFields(text);
      if (fields.empty())
         throw badStl;

      // The 3D model begins with the keyword "solid".
      if (fields[0] != "solid")
      {
         throw "File does not appear to be an ASCII STL file.  Expected keyword \"solid\".\n";
      }
   }

   //--------------------------------------------------------------------
   // Returns true if the given file appears to be a binary STL file.
   //--------------------------------------------------------------------
   bool IsBinaryStl(File &inFile)
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());

      size_t numFacets = 0;
      try
      {
         ReadHeaderFromBinaryStl(numFacets);
      }
      catch(...)
      {
         return false;
      }
      if (numFacets < 1)
         return false;
   
      // Confirm that the file size is correct for the number of facets
      // specified.
      size_t expectedLength = 80 + sizeof(std::uint32_t) +
                                      numFacets * sizeof(BinaryStlRecord);
      return inFile.Length() == expectedLength;
   }

   //--------------------------------------------------------------------
   // Reads a vertex line from the STL file.
   // The x, y, z coordinates of the vertex are returned in "point".
   // Throws zstring if error.
   //--------------------------------------------------------------------
   void ReadVertexFromAsciiStl(Point &point)
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      std::string text;
      if (!m_file.ReadLine(text, true))
         throw "Unexpected end of file while looking for \"vertex\".\n";
   
      auto fields = StringFields(text);
      if (fields.size() < 4)
         throw "Malformed vertex line.  Expected x, y, and z coordinates.\n";

      if (fields[0] != "vertex")
         throw "Missing or malformed vertex.\n";

      // Convert text to doubles.
      point.x = atof(fields[1].c_str());
      point.y = atof(fields[2].c_str());
      point.z = atof(fields[3].c_str());

      // Check for bad number conversions.
      if ((point.x == 0. && fields[1][0] != '0') ||
          (point.y == 0. && fields[2][0] != '0') ||
          (point.z == 0. && fields[3][0] != '0') ||
           _isnan(point.x) || _isnan(point.y) || _isnan(point.z) ||
           !_finite(point.x) || !_finite(point.y) || !_finite(point.z))
      {
         throw "One or more invalid coordinate values after \"vertex\".\n";
      }
   }

   //--------------------------------------------------------------------
   // Reads/parses a facet of the following format:
   //
   // facet normal normalX normalY normalZ
   //    outer loop
   //       vertex x y z
   //       vertex x y z
   //       vertex x y z
   //    end loop
   // end facet
   //
   // The coordinates from the vertex lines are passed back in "coords".
   // Returns false if there are no more facets in the file.
   // Throws zstring if error.
   //--------------------------------------------------------------------
   bool ReadFacetFromAsciiStl(std::vector<Point> &coords)
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      std::string text;
      if (!m_file.ReadLine(text, true))
         return false;

      auto fields = StringFields(text);
      if (fields.empty())
         return false;

      if (fields[0] == "endsolid" || (fields.size() > 1 && fields[0] == "end" && fields[1] == "solid"))
      {
         // This marks the end of the current 3D model,
         // which means there are no more facets to read.
         return false;
      }
      else if (fields[0] == "facet")
      {
         // Read and check the outerloop line.
         if (!m_file.ReadLine(text, true))
         {
            return false;
         }
         fields = StringFields(text);
         if (!((fields.size() > 0 && fields[0] == "outerloop") ||
              (fields.size() > 1 && fields[0] == "outer" && fields[1] == "loop")))
         {
            throw "A facet is missing its outer loop.\n";
         }
      
         // Read and store the three vertex coordinates that describe the face's triangle.
         for (int i = 0; i < 3; ++i)
         {
            Point point;
            ReadVertexFromAsciiStl(point);
            coords.push_back(point);
         }
      
         // Read and check the endloop line.
         if (!m_file.ReadLine(text, true))
            throw "Unexpected end of file while reading a facet.\n";
         fields = StringFields(text);
         if (!((fields.size() >= 1 && fields[0] == "endloop") ||
               (fields.size() >= 2 && fields[0] == "end" && fields[1] == "loop")))
            throw "Expected \"end loop\" after vertex.\n";
      
         // Read and check the endfacet line.
         if (!m_file.ReadLine(text, true))
            throw "Unexpected end of file while reading a facet.\n";
         fields = StringFields(text);
         if (!((fields.size() >= 1 && fields[0] == "endfacet") ||
               (fields.size() >= 2 && fields[0] == "end" && fields[1] == "facet")))
            throw "Expected \"end facet\" after \"end loop\".\n";
      }
      else
      {
         throw "Expected \"facet\" or \"end solid\"\n";
      }

      return true;
   }

   //--------------------------------------------------------------------
   // Reads the next facet from a binary STL file.
   // Returns false if there are no more facets in the file.
   // Errors throw strings.
   //--------------------------------------------------------------------
   bool ReadFacetFromBinaryStl(std::vector<Point> &coords)
   {
      // cppcheck-suppress assertWithSideEffect
      assert(m_file.IsOpen());
      if (m_curFacet >= m_numFacets)
         return false;

      BinaryStlRecord record;
      if (m_file.Read(&record, sizeof(record)) != sizeof(record))
         throw "Failed reading facet record from binary STL file.";
      if (record.m_attributeSize != 0)
         throw "Invalid attribute size in binary STL file.";

      // Build the coordinate list from the record.
      float *fp = &record.m_coorddata[0];
      for (int i = 0; i < 3; ++i)
      {
         Point point;
         point.x = *fp++;
         point.y = *fp++;
         point.z = *fp++;
         coords.push_back(point);
      }

      m_curFacet++;
      return true;
   }

private:
   File     m_file;
   bool     m_isBinaryStl = false;
   size_t   m_numFacets = 0;
   size_t   m_curFacet = 0;
};

//--------------------------------------------------------------------
// Expands the bounds indicated by emin and emax to include the
// given input point.
//--------------------------------------------------------------------
void UpdateMinMax(const Point &input, Point &emin, Point &emax)
{
   if (input.x < emin.x)      emin.x = input.x;
   if (input.y < emin.y)      emin.y = input.y;
   if (input.z < emin.z)      emin.z = input.z;
   if (input.x > emax.x)      emax.x = input.x;
   if (input.y > emax.y)      emax.y = input.y;
   if (input.z > emax.z)      emax.z = input.z;
}

//--------------------------------------------------------------------
// Converts a 3D model from .STL file format to VRML .WRL file format.
// The .STL file may be binary or ASCII STL format.
//--------------------------------------------------------------------
void ConvertStlToWrl(File &inFile, File &outFile)
{
   StlReader reader(inFile);
   reader.ReadHeaderFromStl();

   VrmlWriter writer(outFile);
   writer.WriteStartOfWrl();

   // These two points are used to accumulate the minimum and maximum
   // bounds of the 3D model as we read its coordinates from the STL file.
   Point emin, emax;
   emin.x = emin.y = emin.z = DBL_MAX;
   emax.x = emax.y = emax.z = -DBL_MAX;

   size_t numFacetsProcessed = 0;

   // Read all facets in the STL model and write them to the WRL file.
   std::vector<Point> coords;
   while (reader.ReadFacetFromStl(coords))
   {
      writer.WriteFacetToWrl(coords);
      for (const auto &point : coords)
         UpdateMinMax(point, emin, emax);

      // Periodically output a dot to the console to indicate progress
      // while processing a very large model.
      ++numFacetsProcessed;
      if ((numFacetsProcessed % 1000) == 0)
         fprintf(stderr, ".");
   }
   fprintf(stderr, "\n");

   reader.Close();
   writer.WriteEndOfWrl(emin, emax);
}

//--------------------------------------------------------------------
// Program entry point.  Takes standard arguments from the command
// line and returns EXIT_SUCCESS if no errors occur.
//--------------------------------------------------------------------
int wmain(int argc, wchar_t **argv)
{
   if (argc != 3)
   {
      // The user needs command line help.
      printf("Usage:  stl2vrml infile.stl outfile.wrl\n");
      return EXIT_FAILURE;
   }

   const wchar_t *inFilename = argv[1];
   const wchar_t *outFilename = argv[2];
   fwprintf(stderr, L"Converting %s to %s\n", inFilename, outFilename);

   // Open the STL input file.
   wprintf(L"Opening %s for reading.\n", inFilename);
   File inFile;
   if (!inFile.Open(inFilename))
   {
      wprintf(L"stl2vrml:  Failed opening input file:  %s\n", inFilename);
      return EXIT_FAILURE;
   }

   // Open the WRL output file.
   wprintf(L"stl2vrml:  Opening %s for writing.\n", outFilename);
   File outFile;
   if (!outFile.Create(outFilename))
   {
      wprintf(L"stl2vrml:  Failed opening output file:  %s\n", outFilename);
      return EXIT_FAILURE;
   }

   try
   {
      printf("stl2vrml:  Processing.\n");
      ConvertStlToWrl(inFile, outFile);
   }
   catch(const char *text)
   {
      printf("stl2vrml:  Error - %s\n", text);
      return EXIT_FAILURE;
   }
   catch(...)
   {
      printf("stl2vrml:  Aborted due to exception!\n");
      return EXIT_FAILURE;
   }

   printf("stl2vrml:  Done.\n");
   return EXIT_SUCCESS;
}

