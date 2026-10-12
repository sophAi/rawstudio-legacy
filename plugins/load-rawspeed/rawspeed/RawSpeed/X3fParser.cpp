#include <cstdint>
#include "StdAfx.h"
#include "X3fParser.h"
#include "X3fDecoder.h"
#include "ByteStreamSwap.h"
/*
RawSpeed - RAW file decoder.

Copyright (C) 2009-2013 Klaus Post

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

http://www.klauspost.com
*/

namespace RawSpeed {

X3fParser::X3fParser(FileMap* file) {
  decoder = NULL;
  bytes = NULL;
  mFile = file;
  uint32 size = file->getSize();
  if (size<104+128)
    ThrowRDE("X3F file too small");

  if (getHostEndianness() == little)
    bytes = new ByteStream(file, 0, size);
  else
    bytes = new ByteStreamSwap(file, 0, size);
  try {
    try {
      // Read signature
      if (bytes->getUInt() != 0x62564f46)
        ThrowRDE("X3F Decoder: Not an X3f file (Signature)");

      uint32 version = bytes->getUInt();
      if (version < 0x00020000)
        ThrowRDE("X3F Decoder: File version too old");

      // Skip identifier + mark bits
      bytes->skipBytes(16+4);

      bytes->setAbsoluteOffset(0);
      decoder = new X3fDecoder(file);
      readDirectory();
    } catch (IOException e) {
      ThrowRDE("X3F Decoder: IO Error while reading header: %s", e.what());
    }
  } catch (RawDecoderException e) {
    freeObjects();
    throw e;
  }
}

void X3fParser::freeObjects() {
  if (bytes)
    delete bytes;
  if (decoder)
    delete decoder;
  decoder = NULL;
  bytes = NULL;
}

X3fParser::~X3fParser(void)
{
  freeObjects();
}


static string getIdAsString(ByteStream *bytes) {
  uchar8 id[5];
  for (int i = 0; i < 4; i++)
    id[i] = bytes->getByte();
  id[4] = 0;
  return string((const char*)id);
}


void X3fParser::readDirectory()
{
  bytes->setAbsoluteOffset(mFile->getSize()-4);
  uint32 dir_off = bytes->getUInt();
  bytes->setAbsoluteOffset(dir_off);

  // Check signature
  if (0 != getIdAsString(bytes).compare("SECd"))
    ThrowRDE("X3F Decoder: Unable to locate directory");

  uint32 version = bytes->getUInt();
  if (version < 0x00020000)
    ThrowRDE("X3F Decoder: File version too old (directory)");

  uint32 n_entries = bytes->getUInt();
  for (uint32 i = 0; i < n_entries; i++) {
    X3fDirectory dir(bytes);
    decoder->mDirectory.push_back(dir);
    bytes->pushOffset();
    if (0 == dir.id.compare("IMA2") || 0 == dir.id.compare("IMAG")){
      decoder->mImages.push_back(X3fImage(bytes, dir.offset, dir.length));
    }
    if (0 == dir.id.compare("PROP")){
      decoder->mProperties.addProperties(bytes, dir.offset, dir.length);
    }
    bytes->popOffset();
  }
}

RawDecoder* X3fParser::getDecoder()
{
  if (NULL == decoder)
    ThrowRDE("X3fParser: No decoder found!");
  RawDecoder *ret = decoder;
  decoder = NULL;
  return ret;
}

X3fDirectory::X3fDirectory( ByteStream *bytes )
{
    offset = bytes->getUInt();
    length = bytes->getUInt();
    id = getIdAsString(bytes);
    bytes->pushOffset();
    bytes->setAbsoluteOffset(offset);
    sectionID = getIdAsString(bytes);
    bytes->popOffset();
}


X3fImage::X3fImage( ByteStream *bytes, uint32 offset, uint32 length )
{
  bytes->setAbsoluteOffset(offset);
  string id = getIdAsString(bytes);
  if (id.compare("SECi"))
    ThrowRDE("X3fImage:Unknown Image signature");

  uint32 version = bytes->getUInt();
  if (version < 0x00020000)
    ThrowRDE("X3F Decoder: File version too old (image)");

  type = bytes->getUInt();
  format = bytes->getUInt();
  width = bytes->getUInt();
  height = bytes->getUInt();
  pitchB = bytes->getUInt();
  dataOffset = bytes->getOffset();
  dataSize = length - (dataOffset-offset);
  if (pitchB == dataSize)
    pitchB = 0;
}


/*
* convertUTF16ToUTF8:
* https://chromium.googlesource.com/v8/v8.git/+/refs/heads/main/src/inspector/v8-string-conversions.cc
* 
* Copyright 2006-2019, the V8 project authors. All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Google Inc. nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
* 
*/
using UChar = char16_t;
using UChar32 = uint32_t;

bool isASCII(UChar c) { return !(c & ~0x7F); }

const UChar replacementCharacter = 0xFFFD;

inline int inlineUTF8SequenceLengthNonASCII(char b0) {
  if ((b0 & 0xC0) != 0xC0) return 0;
  if ((b0 & 0xE0) == 0xC0) return 2;
  if ((b0 & 0xF0) == 0xE0) return 3;
  if ((b0 & 0xF8) == 0xF0) return 4;
  return 0;
}

inline int inlineUTF8SequenceLength(char b0) {
  return isASCII(b0) ? 1 : inlineUTF8SequenceLengthNonASCII(b0);
}

// Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
// into the first byte, depending on how many bytes follow.  There are
// as many entries in this table as there are UTF-8 sequence types.
// (I.e., one byte sequence, two byte... etc.). Remember that sequences
// for *legal* UTF-8 will be 4 or fewer bytes total.
static const unsigned char firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0,
                                               0xF0, 0xF8, 0xFC};
                                               
static bool convertUTF16ToUTF8(const UChar** sourceStart,
                                     const UChar* sourceEnd, char** targetStart,
                                     char* targetEnd, bool strict) {
  bool result = TRUE;
  const UChar* source = *sourceStart;
  char* target = *targetStart;
  while (source < sourceEnd) {
    UChar32 ch;
    uint32_t bytesToWrite = 0;
    const UChar32 byteMask = 0xBF;
    const UChar32 byteMark = 0x80;
    const UChar* oldSource =
        source;  // In case we have to back up because of target overflow.
    ch = static_cast<uint16_t>(*source++);
    // If we have a surrogate pair, convert to UChar32 first.
    if (ch >= 0xD800 && ch <= 0xDBFF) {
      // If the 16 bits following the high surrogate are in the source buffer...
      if (source < sourceEnd) {
        UChar32 ch2 = static_cast<uint16_t>(*source);
        // If it's a low surrogate, convert to UChar32.
        if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
          ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x0010000;
          ++source;
        } else if (strict) {  // it's an unpaired high surrogate
          --source;           // return to the illegal value itself
          result = FALSE;
          break;
        }
      } else {     // We don't have the 16 bits following the high surrogate.
        --source;  // return to the high surrogate
        result = FALSE;
        break;
      }
    } else if (strict) {
      // UTF-16 surrogate values are illegal in UTF-32
      if (ch >= 0xDC00 && ch <= 0xDFFF) {
        --source;  // return to the illegal value itself
        result = FALSE;
        break;
      }
    }
    // Figure out how many bytes the result will require
    if (ch < static_cast<UChar32>(0x80)) {
      bytesToWrite = 1;
    } else if (ch < static_cast<UChar32>(0x800)) {
      bytesToWrite = 2;
    } else if (ch < static_cast<UChar32>(0x10000)) {
      bytesToWrite = 3;
    } else if (ch < static_cast<UChar32>(0x110000)) {
      bytesToWrite = 4;
    } else {
      bytesToWrite = 3;
      ch = replacementCharacter;
    }
    target += bytesToWrite;
    if (target > targetEnd) {
      source = oldSource;  // Back up source pointer!
      target -= bytesToWrite;
      result = FALSE;
      break;
    }
    switch (bytesToWrite) {
      case 4:
        *--target = static_cast<char>((ch | byteMark) & byteMask);
        ch >>= 6;
      case 3:
        *--target = static_cast<char>((ch | byteMark) & byteMask);
        ch >>= 6;
      case 2:
        *--target = static_cast<char>((ch | byteMark) & byteMask);
        ch >>= 6;
      case 1:
        *--target = static_cast<char>(ch | firstByteMark[bytesToWrite]);
    }
    target += bytesToWrite;
  }
  *sourceStart = source;
  *targetStart = target;
  return result;
}

string X3fPropertyCollection::getString( ByteStream *bytes ) {
  uint32 max_len = bytes->getRemainSize() / 2;
  const UChar* start = (const UChar*)bytes->getData();
  const UChar* src_end = start;
  uint32 i = 0;
  for (; i < max_len && start == src_end; i++) {
    if (start[i] == 0) {
      src_end = &start[i];
    }
  }
  if (start != src_end) {
    char* dest = new char[i * 4 + 1];
    memset(dest, 0, i * 4 + 1);
    if (convertUTF16ToUTF8(&start, src_end, &dest, &dest[i * 4 - 1], /* strict= */ true)) {
      string ret((const char*)dest);
      delete[] dest;
      return ret;
    }
    delete[] dest;
  }
  return "";
}

void X3fPropertyCollection::addProperties( ByteStream *bytes, uint32 offset, uint32 length )
{
  bytes->setAbsoluteOffset(offset);
  string id = getIdAsString(bytes);
  if (id.compare("SECp"))
    ThrowRDE("X3fImage:Unknown Property signature");

  uint32 version = bytes->getUInt();
  if (version < 0x00020000)
    ThrowRDE("X3F Decoder: File version too old (properties)");

  uint32 entries = bytes->getUInt();
  if (!entries)
    return;

  if (0 != bytes->getUInt())
    ThrowRDE("X3F Decoder: Unknown property character encoding");

  // Skip 4 reserved bytes
  bytes->skipBytes(4);

  // Skip size (not used ATM)
  bytes->skipBytes(4);

  if (entries > 1000)
    ThrowRDE("X3F Decoder: Unreasonable number of properties: %u", entries);

  uint32 data_start = bytes->getOffset() + entries*8;
  for (uint32 i = 0; i < entries; i++) {
    uint32 key_pos = bytes->getUInt();
    uint32 value_pos = bytes->getUInt();
    bytes->pushOffset();
    try {
      bytes->setAbsoluteOffset(key_pos * 2 + data_start);
      string key = getString(bytes);
      bytes->setAbsoluteOffset(value_pos * 2 + data_start);
      string val = getString(bytes);
      props[key] = val;
    } catch (IOException) {}
    bytes->popOffset();
  }
}

} // namespace RawSpeed
