/** @file */
/* -----------------------------------------------------------------------------
   File I/O Library
   © Geoff Crossland 2017
----------------------------------------------------------------------------- */
#ifndef IO_FILE_ALREADYINCLUDED
#define IO_FILE_ALREADYINCLUDED

#include <core.hpp>

namespace io { namespace file {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
extern DC();

/**
  An {@c InputStream} and {@c OutputStream} for files. An instance can be used
  in both roles during its lifetime, but sync() must be called when switching
  from one to another.
*/
class FileStream {
  pub typedef unsigned long Size;
  /**
    Specifies how much access a FileStream should have to its file and what
    should happen during construction to any existing contents of the file.
  */
  pub enum Mode {
    // TODO more modes (extend CREATEs by doing two fopen()s?)
    READ_EXISTING,
    READ_WRITE_EXISTING,
    READ_WRITE_RECREATE,
    APPEND_CREATE,
    READ_APPEND_CREATE
  };

  prv FILE *h;
  DI(prv enum State {
    FREE,
    READING,
    WRITING
  } state;)

  pub FileStream (const core::u8string &pathName, Mode mode);
  FileStream (const FileStream &) = delete;
  FileStream &operator= (const FileStream &) = delete;
  pub FileStream (FileStream &&) noexcept;
  pub FileStream &operator= (FileStream &&) noexcept;
  pub ~FileStream ();

  /**
    Gets the current position in the file.
  */
  pub Size tell () const;
  prv void seek (long offset, int origin);
  /**
    Sets the current position in the file.
  */
  pub void seek (Size offset);
  /**
    Sets the current position in the file to the end.
  */
  pub void seekToEnd ();
  /**
    Must be called before calling read() after having called write() or vice-versa.
  */
  pub void sync ();
  pub size_t read (iu8f *b, size_t s);
  pub void write (const iu8f *b, size_t s);
  pub void close ();
};

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}}

#endif
