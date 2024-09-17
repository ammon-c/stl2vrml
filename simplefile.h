//--------------------------------------------------------------------
// SimpleFile.h - Minimalist generic file class for C++ programs.
//
// (C) Copyright 2007,2018 Ammon R. Campbell.
//
// I wrote this code for use in my own educational and experimental
// programs, but you may also freely use it in yours as long as you
// abide by the following terms and conditions.
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
// Limitations / Bugs:
// * Doesn't work with files over 2GB in size (uses 32-bit offsets).
// * Error handling is weak.
//
//--------------------------------------------------------------------

#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>

class File
{
private:
   FILE *m_file = nullptr;
   size_t m_lineCounter = 0;

public:
   ~File() { if (m_file) fclose(m_file); }

   // Open file for reading.
   bool Open(const wchar_t *filename)
      { return !(_wfopen_s(&m_file, filename, L"rb") || m_file == nullptr); }

   // Open file for writing.
   bool Create(const wchar_t *filename)
      { return !(_wfopen_s(&m_file, filename, L"wb") || m_file == nullptr); }

   // Returns true if the file is currently open.
   bool IsOpen()
      { return (m_file != nullptr); }

   // Close the file.
   void Close() { if (m_file) fclose(m_file); }

   // Seek to specific position in file.
   bool Seek(size_t position)
      { return !fseek(m_file, static_cast<long>(position), SEEK_SET); }

   // Write numbytes of data to file.  Returns true if successful.
   bool Write(const void *data, size_t numbytes)
      { return fwrite(data, numbytes, 1, m_file) == 1; }

   // Read numbytes of data from file.  Returns number of bytes actually read.
   size_t Read(void *data, size_t numbytes)
      { return fread(data, 1, numbytes, m_file); }

   // Reads a line of text from the file.
   // Carriage returns and line feeds are omitted from the text.
   // Returns false if there are no more lines of text in file.
   // If skipBlankLines is true, skips any blank lines in the
   // file before reading a non-blank line.
   bool ReadLine(std::string &text, bool skipBlankLines = false)
   {
      bool readAny = false;
      do
      {
         text.clear();
         char c = '\0';
         while (Read(&c, 1) == 1 && c != '\n')
         {
            readAny = true;
            if (c != '\r')
               text += c;
         }
         if (c == '\n')
            m_lineCounter++;
      }
      while ((skipBlankLines && text.empty()) && readAny);
      return readAny;
   }

   // Return a count of how many lines have been read so far by ReadLine calls.
   size_t LineCounter() const { return m_lineCounter; }

   // Reset the line counter to zero.  The current file position is not changed.
   void ResetLineCounter() { m_lineCounter = 0; }

   // Write formatted text to the file.
   // Maximum length of formatted text is 32Kbytes.
   bool Printf(_Printf_format_string_ const char *fmt, ...)
   {
      std::vector<char> buffer(1024, 0);
      va_list vv;
      va_start(vv, fmt);
      while (buffer.size() < 32768 &&
             vsprintf_s(buffer.data(), buffer.size(), fmt, vv) < 0)
      {
         size_t oldsize = buffer.size();
         buffer.clear();
         buffer.resize(oldsize * 2);
      }
      va_end(vv);
      return Write(buffer.data(), strlen(buffer.data()));
   }

   // Retrieve length of file in bytes.
   // Note:  Returned value will be incorrect for files over 2GB in size.
   size_t Length()
   {
      if (!m_file) return 0;
      size_t pos = ftell(m_file);
      fseek(m_file, 0, SEEK_END);
      size_t endpos = ftell(m_file);
      fseek(m_file, static_cast<long>(pos), SEEK_SET);
      return endpos;
   }
};

