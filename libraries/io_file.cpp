#include "io_file.hpp"
#include <cstring>

namespace io::file {

using core::u8string;
using core::PlainException;
using std::move;
using core::unsign;
using core::numeric_limits;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

u8string createStrerror (int errnum, const char8_t *prefix = u8" (", const char8_t *suffix = u8")") {
  u8string msg;
  if (errnum) {
    char b[1024];
    // TODO handle text properly (encoding, initial letter capitalisation)
    // TODO transplant this and core::createExceptionMessage() to somewhere after local encoding handling (umain, setlocale calling etc.)
    auto r = strerror_r(errnum, b, sizeof(b) / sizeof(*b));
    const char *s;
    #ifdef _GNU_SOURCE
    s = r;
    #else
    s = r == 0 ? b : nullptr;
    #endif

    if (s && s[0] != u8'\0') {
      msg += prefix;
      msg += reinterpret_cast<const char8_t *>(s);
      msg += suffix;
    }
  }
  return msg;
}

FileStream::FileStream (const u8string &pathName, Mode mode) {
  const char *m;
  switch (mode) {
    case Mode::readExisting:
      m = "rb";
      break;
    case Mode::readWriteExisting:
      m = "r+b";
      break;
    case Mode::readWriteRecreate:
      m = "w+b";
      break;
    case Mode::appendCreate:
      m = "ab";
      break;
    case Mode::readAppendCreate:
      m = "a+b";
      break;
    default:
      DPRE(false);
      m = "";
  }

  errno = 0;
  // TODO handle path name correctly
  h = fopen(reinterpret_cast<const char *>(pathName.c_str()), m);
  if (!h) {
    throw PlainException(u8string(u8"failed to open '") + pathName + u8"'" + createStrerror(errno));
  }
  DI(state = State::free;)

  setbuf(h, NULL);
}

FileStream::FileStream (FileStream &&o) noexcept : h(nullptr) {
  *this = move(o);
}

FileStream &FileStream::operator= (FileStream &&o) noexcept {
  if (this != &o) {
    if (h) {
      fclose(h);
    }
    h = o.h;
    o.h = nullptr;
    DI(state = o.state;)
  }
  return *this;
}

FileStream::~FileStream () {
  if (h) {
    fclose(h);
  }
}

FileStream::Size FileStream::tell () const {
  long offset = ftell(h);
  if (offset == -1) {
    throw PlainException(u8string(u8"failed to retrieve current position in file") + createStrerror(errno));
  }
  return unsign(offset);
}

void FileStream::seek (long offset, int origin) {
  errno = 0;
  int r = fseek(h, offset, origin);
  DI(state = State::free;)
  if (r != 0) {
    throw PlainException(u8string(u8"failed to set current position in file") + createStrerror(errno));
  }
}

void FileStream::seek (Size offset) {
  if (offset > unsign(numeric_limits<long>::max())) {
    throw PlainException(u8string(u8"failed to set current position in file (requested position was too big)"));
  }
  seek(static_cast<long>(offset), SEEK_SET);
}

void FileStream::seekToEnd () {
  seek(0, SEEK_END);
}

void FileStream::sync () {
  // TODO how does this perform for streams?
  seek(0, SEEK_CUR);
}

size_t FileStream::read (iu8f *b, size_t s) {
  DPRE(state == State::free || state == State::reading);
  DPRE(s < numeric_limits<size_t>::max());
  if (s == 0) {
    return 0;
  }

  DI(state = State::reading;)
  errno = 0;
  size_t outSize = fread(b, 1, s, h);
  if (outSize == 0) {
    if (feof(h)) {
      return numeric_limits<size_t>::max();
    }
    DA(ferror(h));
    throw PlainException(u8string(u8"failed to read from file") + createStrerror(errno));
  }

  return outSize;
}

void FileStream::write (const iu8f *b, size_t s) {
  DPRE(state == State::free || state == State::writing);
  DI(state = State::writing;)
  errno = 0;
  size_t outSize = fwrite(b, 1, s, h);
  if (outSize != s) {
    throw PlainException(u8string(u8"failed to write to file") + createStrerror(errno));
  }
}

void FileStream::close () {
  if (!h) {
    return;
  }

  int r = fclose(h);
  h = nullptr;
  if (r != 0) {
    throw PlainException(u8string(u8"failed to close file") + createStrerror(errno));
  }
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}
