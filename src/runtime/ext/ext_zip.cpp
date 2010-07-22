/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010 Facebook, Inc. (http://www.facebook.com)          |
   | Copyright (c) 1997-2010 The PHP Group                                |
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

#include <runtime/ext/ext_zip.h>
#include <runtime/ext/ext_file.h>
#include <runtime/ext/ext_string.h>
#include <string.h>

// update ZipArchive's status and statusSys properties
#define UPDATE_STATUS zip_error_get(m_archive, &m_status, &m_statusSys)

// extension of CREATE_MAP7
#define CREATE_MAP7(n1, e1, n2, e2, n3, e3, n4, e4, n5, e5, n6, e6, n7, e7)       \
  Array(ArrayInit(7, false).set(0, n1, e1).set(1, n2, e2).set(2, n3, e3). \
                            set(3, n4, e4).set(4, n5, e5).set(5, n6, e6). \
                            set(6, n7, e7).create())

#define CHECK_ZAR_HANDLE(handle, f)                         \
  ZipArchiveResource *f = handle.getTyped<ZipArchiveResource>(true, true);          \
  if (f == NULL) {                                      \
    raise_warning("Not a valid zip archive resource");       \
  }                                                     \

#define CHECK_ZEF_HANDLE(handle, f)                         \
  ZipEntryFile *f = handle.getTyped<ZipEntryFile>(true, true);          \
  if (f == NULL) {                                      \
    raise_warning("Not a valid zip entry stream");       \
  }                                                     \

namespace HPHP {

// needed for extension_loaded('zip') to work
IMPLEMENT_DEFAULT_EXTENSION(zip);

///////////////////////////////////////////////////////////////////////////////

// passing constants from libzip
const int64 q_ziparchive_CREATE = ZIP_CREATE;
const int64 q_ziparchive_EXCL = ZIP_EXCL;
const int64 q_ziparchive_CHECKCONS = ZIP_CHECKCONS;

// OVERWRITE is shortcutted since it's not implemented 
// in lower version libzip
const int64 q_ziparchive_OVERWRITE = 8; 
const int64 q_ziparchive_FL_NOCASE = ZIP_FL_NOCASE;
const int64 q_ziparchive_FL_NODIR = ZIP_FL_NODIR;
const int64 q_ziparchive_FL_COMPRESSED = ZIP_FL_COMPRESSED;
const int64 q_ziparchive_FL_UNCHANGED = ZIP_FL_UNCHANGED;
const int64 q_ziparchive_CM_DEFAULT = ZIP_CM_DEFAULT;
const int64 q_ziparchive_CM_STORE = ZIP_CM_STORE;
const int64 q_ziparchive_CM_SHRINK = ZIP_CM_SHRINK;
const int64 q_ziparchive_CM_REDUCE_1 = ZIP_CM_REDUCE_1;
const int64 q_ziparchive_CM_REDUCE_2 = ZIP_CM_REDUCE_2;
const int64 q_ziparchive_CM_REDUCE_3 = ZIP_CM_REDUCE_3;
const int64 q_ziparchive_CM_REDUCE_4 = ZIP_CM_REDUCE_4;
const int64 q_ziparchive_CM_IMPLODE = ZIP_CM_IMPLODE;
const int64 q_ziparchive_CM_DEFLATE = ZIP_CM_DEFLATE;
const int64 q_ziparchive_CM_DEFLATE64 = ZIP_CM_DEFLATE64;
const int64 q_ziparchive_CM_PKWARE_IMPLODE = ZIP_CM_PKWARE_IMPLODE;
const int64 q_ziparchive_ER_OK = ZIP_ER_OK;
const int64 q_ziparchive_ER_MULTIDISK = ZIP_ER_MULTIDISK;
const int64 q_ziparchive_ER_RENAME = ZIP_ER_RENAME;
const int64 q_ziparchive_ER_CLOSE = ZIP_ER_CLOSE;
const int64 q_ziparchive_ER_SEEK = ZIP_ER_SEEK;
const int64 q_ziparchive_ER_READ = ZIP_ER_READ;
const int64 q_ziparchive_ER_WRITE = ZIP_ER_WRITE;
const int64 q_ziparchive_ER_CRC = ZIP_ER_CRC;
const int64 q_ziparchive_ER_ZIPCLOSED = ZIP_ER_ZIPCLOSED;
const int64 q_ziparchive_ER_NOENT = ZIP_ER_NOENT;
const int64 q_ziparchive_ER_EXISTS = ZIP_ER_EXISTS;
const int64 q_ziparchive_ER_OPEN = ZIP_ER_OPEN;
const int64 q_ziparchive_ER_TMPOPEN = ZIP_ER_TMPOPEN;
const int64 q_ziparchive_ER_ZLIB = ZIP_ER_ZLIB;
const int64 q_ziparchive_ER_MEMORY = ZIP_ER_MEMORY;
const int64 q_ziparchive_ER_CHANGED = ZIP_ER_CHANGED;
const int64 q_ziparchive_ER_COMPNOTSUPP = ZIP_ER_COMPNOTSUPP;
const int64 q_ziparchive_ER_EOF = ZIP_ER_EOF;
const int64 q_ziparchive_ER_INVAL = ZIP_ER_INVAL;
const int64 q_ziparchive_ER_NOZIP = ZIP_ER_NOZIP;
const int64 q_ziparchive_ER_INTERNAL = ZIP_ER_INTERNAL;
const int64 q_ziparchive_ER_INCONS = ZIP_ER_INCONS;
const int64 q_ziparchive_ER_REMOVE = ZIP_ER_REMOVE;
const int64 q_ziparchive_ER_DELETED = ZIP_ER_DELETED;

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
// implementation of f_zip_* functions
// the internal zip_file resource is freed through zip_fclose() only in 
// f_zip_close() and f_zip_read(). f_zip_entry_open() and f_zip_entry_close()
// only query status but take no operation.

void f_zip_close(CObjRef zip) {
  CHECK_ZAR_HANDLE(zip, f);  
  f->close();
}

bool f_zip_entry_close(CObjRef zip_entry) {
  CHECK_ZEF_HANDLE(zip_entry, f);  
  // similar to f_zip_entry_open, only return status, no operation 
  return f->setClose();
}

int64 f_zip_entry_compressedsize(CObjRef zip_entry) {
  CHECK_ZEF_HANDLE(zip_entry, f);  
  return f->getCompressedSize();
}

String f_zip_entry_compressionmethod(CObjRef zip_entry) {
  CHECK_ZEF_HANDLE(zip_entry, f);  
  return f->getCompressionMethod();
}

int64 f_zip_entry_filesize(CObjRef zip_entry) {
  CHECK_ZEF_HANDLE(zip_entry, f);  
  return f->getFileSize();
}

String f_zip_entry_name(CObjRef zip_entry) {
  CHECK_ZEF_HANDLE(zip_entry, f);  
  return f->getName();
}

bool f_zip_entry_open(CObjRef zip, CObjRef zip_entry, CStrRef mode /* = null_string */) {
  CHECK_ZEF_HANDLE(zip_entry, f);  
  return f->isOpened();
}

Variant f_zip_entry_read(CObjRef zip_entry, int64 length /* = 1024 */) {
  CHECK_ZEF_HANDLE(zip_entry, f);  

  // sanity check
  if (length <= 0) length = 1024;
  return f->entryRead(length);
}

Variant f_zip_open(CStrRef filename) {
  ZipArchiveResource *zar = NEW(ZipArchiveResource)();
  Object handle(zar);
  zar->open(filename);
  return handle;
}

Variant f_zip_read(CObjRef zip) {
  CHECK_ZAR_HANDLE(zip, f);  
  return f->read();
}

///////////////////////////////////////////////////////////////////////////////
// ZipArchive

c_ziparchive::c_ziparchive () : m_archive(NULL)  {
  m_numFiles=0;
  m_status=0;
  m_statusSys=0;
  m_comment="";
  m_filename="";
}

c_ziparchive::~c_ziparchive() {

  if (m_archive && zip_close(m_archive)) free(m_archive);
}

void c_ziparchive::t___construct() {
}

bool c_ziparchive::t_addemptydir(CStrRef dirname) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::addemptydir);
  // be robust
  String dir_name = f_trim(dirname);

  // take dirname without trailing '/'
  if (dir_name[dir_name.length()-1]!='/') dir_name+="/";
  
  // check if dir already exists
  struct zip_stat sb;
  int idx = zip_stat(m_archive, dir_name, 0, &sb);
  if (idx >= 0) {
    UPDATE_STATUS;
    return false;
  }

  // otherwise would return wrong this->status (9) from previous zip_stat
  zip_error_clear(m_archive);

  int ret;
  if ((ret = zip_add_dir(m_archive, dir_name)) == -1) {
    UPDATE_STATUS;
    return false;
  } else {
    UPDATE_STATUS;
    m_numFiles++;
    // filename is lost upon adding/deleting/updating
    m_filename="";
    return true;
  }
}

bool c_ziparchive::t_addfile(CStrRef filename, CStrRef localname /* = null_string */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::addfile);
  struct zip_source * source = zip_source_file(m_archive, filename.data(), 0, 0);
  int ret;
  String name;
  if (source) {
    if (localname != null_string) 
      name = localname;
    else
      name = filename;

    // let zip_close handle resource releasing 
    // zip_source_free(source);
    ret = zip_add(m_archive, name, source);
    UPDATE_STATUS;
    if (ret == -1)
      return false;
    m_numFiles++;
    // filename is lost upon adding/deleting/updating
    m_filename="";
    return true;
  } else {
    UPDATE_STATUS;
    return false;
  }
}

bool c_ziparchive::t_addfromstring(CStrRef localname, CStrRef contents) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::addfromstring);
  // I suspect buf's content was changed so make a copy
  // and buf is passed into m_archive so let zip_close() release the memory
  char * buf = (char *) malloc(contents.length());
  memcpy(buf, contents.c_str(), contents.length());
  struct zip_source * source = zip_source_buffer(m_archive, buf, contents.length(), 1);

  if (!source) {
    free(buf);
    UPDATE_STATUS;
    return false;
  }
  int ret = zip_add(m_archive, localname, source);

  // zip_source_free(source);
  UPDATE_STATUS;
  if (ret == -1) return false;
  m_numFiles++;
  // filename is lost upon adding/deleting/updating
  m_filename="";
  return true;
}

bool c_ziparchive::t_close() {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::close);
  m_numFiles=0;
  m_filename="";

  if (!m_archive) return false;
  else if (zip_close(m_archive)) {
    free(m_archive);
    m_archive = NULL;
    return false;
  } else { 
    m_archive = NULL;
    return true;
  }
}

bool c_ziparchive::t_deleteindex(int64 index) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::deleteindex);
  const char * entryname = zip_get_name(m_archive, index, ZIP_FL_COMPRESSED);
  if (!entryname) {
    UPDATE_STATUS;
    return false;
  }
  String name = entryname;
  if (zip_delete(m_archive, index)) {
    UPDATE_STATUS;
    return false;
  }
  UPDATE_STATUS;
  m_numFiles--;
  // filename is lost upon adding/deleting/updating
  m_filename="";
  return true;
}

bool c_ziparchive::t_deletename(CStrRef name) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::deletename);
  int index;
  if ((index = zip_name_locate(m_archive, name, 0)) != -1) {
    if (zip_delete(m_archive, index)) {
      UPDATE_STATUS;
      return false;
    }
    UPDATE_STATUS;
    m_numFiles--;
    // filename is lost upon adding/deleting/updating
    m_filename="";
    return true;
  } else {
    UPDATE_STATUS;
    return false;
  }
}

bool c_ziparchive::t_extractto(CStrRef destination, CVarRef entries /* = null_variant */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::extractto);
  String filename; 
  Variant fp, data;
  String path, dirname;
  const char * name;
  int num, i;
  Array files;

  // add trailing '/' to destination string
  String dest_dir = (destination[destination.length()-1]=='/')? destination : destination + "/";

  if (entries.isString()) 
    // string passed in: add to array
    files.append(entries.toString()); 
  else if (entries.isArray()) {
    // array passed in: use the array 
    files=entries.toArray();
  } else if (entries == null_variant) {  
    // third argument ignored: read all files/dirs from zip archive
    num = zip_get_num_files(m_archive);
    for (i=0; i<num; i++) {
      name = zip_get_name(m_archive, i, ZIP_FL_COMPRESSED);
      if (name)
        files.append(String(name));
    }
  } else {
    // unrecognized third argument: return false
    return false;
  }

  num = files.size();
  for (i=0; i<num; i++) {

    // sanity check
    if (!files[i].isString()) continue;

    filename = files[i].toString();
    if (zip_name_locate(m_archive, filename, 0) == -1) {
      // should take dirname without trailing '/' as input
      if (zip_name_locate(m_archive, filename+"/", 0) != -1) {
        filename = filename + "/";
      } else continue;
    }

    filename = dest_dir + files[i];
    // translate into absolute path in system
    path = File::TranslatePath(filename);
    if (path[path.length()-1]=='/') {
      // mkdir if it's a dir and make directory recursively
      if (!f_mkdir(path, 0755, true)) {
        UPDATE_STATUS;
        return false;
      }
      else continue;
    }

    dirname = f_dirname(path);
    // prepare the directory before unziping the file 
    if (!f_mkdir(dirname, 0755, true)) {
      UPDATE_STATUS;
      return false;
    }
    data = t_getfromname(files[i].toString());
    if (!data) {
      UPDATE_STATUS;
      return false; 
    }
    fp = f_fopen(path, "w"); 
    if (!fp) {
      UPDATE_STATUS;
      return false;
    }
    if (!f_fwrite(fp, data)) {
      f_fclose(fp);
      UPDATE_STATUS;
      return false; 
    }
    if (!f_fclose(fp)) {
      UPDATE_STATUS;
      return false;
    }
  }
  UPDATE_STATUS;
  return true;
}

Variant c_ziparchive::t_getarchivecomment() {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::getarchivecomment);
  return m_comment;
}

Variant c_ziparchive::t_getcommentindex(int64 index, int64 flags /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::getcommentindex);
  int lenp;
  const char * comment = zip_get_file_comment(m_archive, index, &lenp, flags); 
  UPDATE_STATUS;
  if (!comment) return false;
  return String(comment, lenp, CopyString);
}

Variant c_ziparchive::t_getcommentname(CStrRef name, int64 flags /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::getcommentname);
  int lenp, index;
  // allocate the entry's index in archive 
  if ((index = zip_name_locate(m_archive, name, 0)) != -1) {
    const char * comment = zip_get_file_comment(m_archive, index, &lenp, flags); 
    UPDATE_STATUS;
    if (!comment) return false;
    return String(comment, lenp, CopyString);
  } else return false;
}

Variant c_ziparchive::t_getfromindex(int64 index, int64 flags /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::getfromindex);
  struct zip_file * zip;
  zip = zip_fopen_index(m_archive, index, flags);
  if (!zip) {
    UPDATE_STATUS;
    return false;
  }

  // read uncompressed file length from zip_stat struct
  struct zip_stat sb;
  zip_stat_index(m_archive, index, flags, &sb);
  char * buff = (char *) malloc(sb.size);
  int len = zip_fread(zip, buff, sb.size);
  if (len == -1 || zip_fclose(zip)) {
    free(buff);
    UPDATE_STATUS;
    return false;
  }
  UPDATE_STATUS;
  return String(buff, len, AttachString);
}

Variant c_ziparchive::t_getfromname(CStrRef name, int64 flags /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::getfromname);
  struct zip_file * zip;
  zip = zip_fopen(m_archive, name.data(), flags);
  if (!zip) {
    UPDATE_STATUS;
    return false;
  }

  // read uncompressed file length from zip_stat struct
  struct zip_stat sb;
  int index = zip_name_locate(m_archive, name, 0);
  zip_stat_index(m_archive, index, flags, &sb);
  char * buff = (char *) malloc(sb.size);
  int len = zip_fread(zip, buff, sb.size);
  if (len == -1 || zip_fclose(zip)) {
    free(buff);
    UPDATE_STATUS;
    return false;
  }
  UPDATE_STATUS;
  return String(buff, len, AttachString);
}

Variant c_ziparchive::t_getnameindex(int64 index) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::getnameindex);
  const char * name = zip_get_name(m_archive, index, ZIP_FL_COMPRESSED);
  UPDATE_STATUS;
  if (name) return String(name);
  else return false;
}

Variant c_ziparchive::t_getstatusstring() {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::getstatusstring);
  int zep, syp, len;
  char error_string[128];

  // use utility functions from libzip to generate status string
  zip_error_get(m_archive, &zep, &syp);
  len = zip_error_to_str(error_string, 128, zep, syp);
  UPDATE_STATUS;
  return String(error_string, len, CopyString);
}

Variant c_ziparchive::t_getstream(CStrRef name) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::getstream);
  struct zip_file * zip;
  zip = zip_fopen(m_archive, name.data(), 0);
  if (!zip) {
    UPDATE_STATUS;
    return false;
  }
  struct zip_stat sb;
  int ret = zip_stat(m_archive, name, 0, &sb);
  if (ret) {
    UPDATE_STATUS;
    return false;
  }

  ZipEntryFile *za = NEW(ZipEntryFile)();
  Object handle(za);
  za->openStream(zip);
  UPDATE_STATUS;
  return handle;
}

Variant c_ziparchive::t_locatename(CStrRef name, int64 flags /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::locatename);
  int ret = zip_name_locate(m_archive, name, flags);
  UPDATE_STATUS;
  if (ret==-1) return false;
  else return ret;
}

Variant c_ziparchive::t_open(CStrRef filename, int64 flags /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::open);
  int errorp;

  // release old before allocating new
  if (m_archive && zip_close(m_archive)) {
      UPDATE_STATUS;
      free(m_archive);
      return false;
  }

  // PHP raises a warning when filename is empty, so we do it too
  if (filename=="")
    raise_warning("ZipArchive::open can't take empty filename");

  m_archive = zip_open(filename.c_str(), flags, &errorp);
  if (!m_archive) return errorp;

  // update comment string
  int lenp;
  const char * comment = zip_get_archive_comment(m_archive, &lenp, ZIP_FL_COMPRESSED); 
  if (comment) m_comment = String(comment, lenp, CopyString);
  else m_comment = "";

  // update numFiles and filename properties
  int num = zip_get_num_files(m_archive);
  UPDATE_STATUS;
  m_numFiles = num;
  m_filename = File::TranslatePath(filename);
  return true;
}

bool c_ziparchive::t_renameindex(int64 index, CStrRef newname) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::renameindex);
  if (zip_rename(m_archive, index, newname)) {
    UPDATE_STATUS;
    return false;
  } else {
    UPDATE_STATUS;
    // filename is lost upon adding/deleting/updating
    m_filename="";
    return true;
  }
}

bool c_ziparchive::t_renamename(CStrRef name, CStrRef newname) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::renamename);
  int index;
  if ((index = zip_name_locate(m_archive, name, 0)) != -1) {
    if (zip_rename(m_archive, index, newname)) {
      UPDATE_STATUS;
      return false;
    } else {
      UPDATE_STATUS;
      // filename is lost upon adding/deleting/updating
      m_filename="";
      return true;
    }
  } else return false;
}

Variant c_ziparchive::t_setarchivecomment(CStrRef comment) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::setarchivecomment);
  int ret;
  if (comment.length() == 0)
    ret = zip_set_archive_comment(m_archive, NULL, 0);  
  else
    ret = zip_set_archive_comment(m_archive, comment.c_str(), comment.length());  
  UPDATE_STATUS;
  if (ret) return false;
  else {
    m_comment = comment;
    // filename is lost upon adding/deleting/updating
    m_filename="";
    return true;
  }
}

Variant c_ziparchive::t_setcommentindex(int64 index, CStrRef comment) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::setcommentindex);
  int ret;
  if (comment.length())
    ret = zip_set_file_comment(m_archive, index, comment, comment.length()); 
  else
    ret = zip_set_file_comment(m_archive, index, NULL, 0); 
  UPDATE_STATUS;
  if (ret) return false;
  else {
    // filename is lost upon adding/deleting/updating
    m_filename="";
    return true;
  }
}

Variant c_ziparchive::t_setcommentname(CStrRef name, CStrRef comment) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::setcommentname);
  int index;
  if ((index = zip_name_locate(m_archive, name, 0)) != -1) {
    int ret;
    if (comment.length())
      ret = zip_set_file_comment(m_archive, index, comment, comment.length()); 
    else
      ret = zip_set_file_comment(m_archive, index, NULL, 0); 
    UPDATE_STATUS;
    if (ret) return false;
    else {
      // filename is lost upon adding/deleting/updating
      m_filename="";
      return true;
    }
  } else return false;
}

Variant c_ziparchive::t_statindex(int64 index, int64 flags /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::statindex);
  struct zip_stat sb;
  int ret = zip_stat_index(m_archive, index, flags, &sb);
  UPDATE_STATUS;
  if (ret) return false;

  // return an array
  return CREATE_MAP7("name", sb.name,
                     "index", sb.index, 
                     "crc", (int64)(sb.crc), 
                     "size", sb.size, 
                     "mtime", sb.mtime, 
                     "comp_size", sb.comp_size, 
                     "comp_method", sb.comp_method);
}

Variant c_ziparchive::t_statname(CStrRef name, int64 flags /* = 0 */) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::statname);
  struct zip_stat sb;
  int ret = zip_stat(m_archive, name, flags, &sb);
  UPDATE_STATUS;
  if (ret) return false;

  // return an array
  return CREATE_MAP7("name", sb.name,
                     "index", sb.index, 
                     "crc", (int64)(sb.crc), 
                     "size", sb.size, 
                     "mtime", sb.mtime, 
                     "comp_size", sb.comp_size, 
                     "comp_method", sb.comp_method);

}

Variant c_ziparchive::t_unchangeall() {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::unchangeall);
  // filename property is lost and cannot be unchanged
  if (zip_unchange_all(m_archive)) {
    UPDATE_STATUS;
    return false;
  } else {
    UPDATE_STATUS;
    return true;
  }
}

Variant c_ziparchive::t_unchangearchive() {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::unchangearchive);
  // filename property is lost and cannot be unchanged
  if (zip_unchange_archive(m_archive)) {
    UPDATE_STATUS;
    return false;
  } else {
    UPDATE_STATUS;
    return true;
  }
}

Variant c_ziparchive::t_unchangeindex(int64 index) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::unchangeindex);
  // filename property is lost and cannot be unchanged
  if (zip_unchange(m_archive, index)) {
    UPDATE_STATUS;
    return false;
  } else {
    UPDATE_STATUS;
    return true;
  }
}

Variant c_ziparchive::t_unchangename(CStrRef name) {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::unchangename);
  // filename property is lost and cannot be unchanged
  int index;
  if ((index = zip_name_locate(m_archive, name, 0)) != -1) {
    if (zip_unchange(m_archive, index)) {
      UPDATE_STATUS;
      return false;
    } else {
      UPDATE_STATUS;
      return true;
    }
  } else return false;
}

Variant c_ziparchive::t___destruct() {
  INSTANCE_METHOD_INJECTION_BUILTIN(ziparchive, ziparchive::__destruct);
  return null;
}

}
