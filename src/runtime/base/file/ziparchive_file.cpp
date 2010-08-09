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

#include <runtime/base/file/ziparchive_file.h>
#include <string.h>

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_OBJECT_ALLOCATION(ZipEntryFile);

ZipEntryFile::ZipEntryFile(): m_zaFile(NULL), m_eof(false), m_sb(NULL), m_entry_opened(false) {
}

// called when sweeping to free resources
ZipEntryFile::~ZipEntryFile() {
  if (m_zaFile) closeImpl();
}

bool ZipEntryFile::open(CStrRef filename, CStrRef mode) {
  ASSERT(m_zaFile == NULL);
  return  true;
}

void ZipEntryFile::openStream(struct zip_file * file) {
  ASSERT(m_zaFile == NULL);
  m_eof = false;
// m_zaFile should be freed from outside only
//  if (m_zaFile) zip_fclose(m_zaFile);
  m_zaFile = file;
  m_entry_opened = true;
}

// stat struct of the entry is passed right after openStream
void ZipEntryFile::setSb(const struct zip_stat * sb) {
  ASSERT(m_zaFile == NULL);
  if (sb) {
  // free previously allocated m_sb
    if (m_sb) free(m_sb);
    m_sb = (struct zip_stat *) malloc(sizeof(struct zip_stat));
    memcpy(m_sb, sb, sizeof(struct zip_stat));
  }
}

bool ZipEntryFile::close() {
  return closeImpl();
}

bool ZipEntryFile::flush() {
  ASSERT(m_zaFile);
  return true;
}  

int64 ZipEntryFile::readImpl(char * buf, int64 length) {
  ASSERT(m_zaFile);
  int len = zip_fread(m_zaFile, buf, length); 
  if (len <= 0) m_eof = true;
  return len;
}

Variant ZipEntryFile::entryRead(int64 length) {
  ASSERT(m_zaFile);
  char * buf  = (char *) malloc(length);
  int len = zip_fread(m_zaFile, buf, length); 
  if (len <= 0) {
    m_eof = true;
    // return false upon eof()
    return false;
  }
  // use AttachString to free buf later
  else return String(buf, len, AttachString); 
}

int64 ZipEntryFile::writeImpl(const char * buf, int64 length) {
  ASSERT(m_zaFile);
  // not implemented in libzip
  return 0;
}

bool ZipEntryFile::closeImpl() {
  ASSERT(m_zaFile);
  bool ret = true;
  if (m_sb) {
    free(m_sb);
    m_sb = NULL;
  }

  // needed for stream
  zip_fclose(m_zaFile);
  m_zaFile = NULL;
  File::closeImpl();
  m_eof = false;
  m_entry_opened = false;
  return ret;
}

// pseudo close used in zip_entry_close
bool ZipEntryFile::setClose() {
  ASSERT(m_zaFile);
  bool ret = true;
  if (m_sb) {
    free(m_sb);
    m_sb = NULL;
  }
  m_zaFile = NULL;
  m_eof = false;
  m_entry_opened = false;
  return ret;
}

bool ZipEntryFile::eof() {
  ASSERT(m_zaFile);
  return m_eof;
}

bool ZipEntryFile::isOpened() {
  return m_entry_opened;
}

String ZipEntryFile::getName() {
  ASSERT(m_sb);
  // make a copy
  return String(m_sb->name, strlen(m_sb->name), CopyString);
}

String ZipEntryFile::getCompressionMethod() {
  ASSERT(m_sb);
  String ret;

  // string mapping copied from PHP's code
	switch (m_sb->comp_method) {
		case 0:
			ret = "stored";
			break;
		case 1:
			ret = "shrunk";
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			ret = "reduced";
			break;
		case 6:
			ret = "imploded";
			break;
		case 7:
			ret = "tokenized";
			break;
		case 8:
			ret = "deflated";
			break;
		case 9:
			ret = "deflatedX";
			break;
		case 10:
			ret = "implodedX";
			break;
		default:
      ret = "";
	}
  return ret;
}

int64 ZipEntryFile::getCompressedSize() {
  ASSERT(m_sb);
  return m_sb->comp_size;
}

int64 ZipEntryFile::getFileSize() {
  ASSERT(m_sb);
  return m_sb->size;
}

///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_OBJECT_ALLOCATION(ZipArchiveResource);

ZipArchiveResource::ZipArchiveResource() : m_zip(NULL), cursor(0), m_zipfile(NULL), m_zef(NULL) {
}

ZipArchiveResource::~ZipArchiveResource() {
  close();
}

Variant ZipArchiveResource::open(CStrRef filename) {
  int errorp;

  // release old resource
  if (m_zip) zip_close(m_zip);

  cursor=0;
  m_zip = zip_open(filename, 0, &errorp);
  if (!m_zip) return errorp;
  else return true;
}

void ZipArchiveResource::close() {
  // close current zip_file before closing the archive
  if (m_zipfile) {
    zip_fclose(m_zipfile);
    // avoid being swept by GC 
    m_zef->openStream(NULL);
    m_zef = NULL;
    m_zipfile = NULL;
  }

  if (m_zip) zip_close(m_zip);
  m_zip = NULL;
}

Variant ZipArchiveResource::read() {
  // release old resource before allocating new
  if (m_zipfile) {
    zip_fclose(m_zipfile);
    // avoid being swept by GC 
    m_zef->openStream(NULL);
    m_zef = NULL;
    m_zipfile = NULL;
  }

  struct zip_stat sb;
  int ret = zip_stat_index(m_zip, cursor, 0, &sb);
  if (ret) return false;

  ZipEntryFile *za = NEW(ZipEntryFile)();
  Object handle(za);
  za->setSb(&sb);

  struct zip_file * zip;
  zip = zip_fopen_index(m_zip, cursor, 0);
  if (!zip) return false;

  m_zipfile = zip;
  m_zef = za;

  // passing m_zipfile to the returned ZipEntryFile instant 
  za->openStream(zip); 

  cursor++;
  return handle;
}

///////////////////////////////////////////////////////////////////////////////
}


