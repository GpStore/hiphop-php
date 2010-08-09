/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010 Facebook, Inc. (http://www.facebook.com)          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef __ZIPARCHIVE_FILE_H__
#define __ZIPARCHIVE_FILE_H__

#include <runtime/base/file/plain_file.h>
#include <zip.h>

namespace HPHP {

///////////////////////////////////////////////////////////////////////////////
// ZipEntryFile class is a crippled stream to implement compatibility 
// functions f_zip_entry_*. 

class ZipEntryFile : public File {
public:
  DECLARE_OBJECT_ALLOCATION(ZipEntryFile);

  const char *o_getClassName() const { return "ZipEntryFile";}

  ZipEntryFile();
  virtual ~ZipEntryFile();

  bool open(CStrRef filename, CStrRef mode); 
  void openStream(struct zip_file * file);
  void setSb(const struct zip_stat * sb);
  bool close(); 
  virtual bool flush();  
  virtual int64 readImpl(char * buf, int64 length); 
  Variant entryRead(int64 length);
  virtual int64 writeImpl(const char * buf, int64 length);
  virtual bool eof();
  bool isOpened();
  bool setClose();

  String getName();
  String getCompressionMethod();
  int64 getFileSize();
  int64 getCompressedSize();

private:
  struct zip_file * m_zaFile;
  bool m_eof;
  bool closeImpl(); 
  struct zip_stat * m_sb;
  bool m_entry_opened;
};

///////////////////////////////////////////////////////////////////////////////
// ZipArchiveResource class

class ZipArchiveResource : public SweepableResourceData {
public:
  DECLARE_OBJECT_ALLOCATION(ZipArchiveResource);

  const char *o_getClassName() const { return "ZipArchiveResource";}

  ZipArchiveResource();
  ~ZipArchiveResource();

  Variant open(CStrRef filename);
  void close();
  Variant read();

private:
  // none is smart-pointer based
  struct zip *m_zip;
  int cursor;
  struct zip_file *m_zipfile;
  ZipEntryFile *m_zef; 
};

///////////////////////////////////////////////////////////////////////////////
}
#endif // __ZIPARCHIVE_FILE_H__

