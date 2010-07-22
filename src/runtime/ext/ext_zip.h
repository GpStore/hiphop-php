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

#ifndef __EXT_ZIP_H__
#define __EXT_ZIP_H__

// >>>>>> Generated by idl.php. Do NOT modify. <<<<<<

#include <runtime/base/base_includes.h>
#include <zip.h>

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

void f_zip_close(CObjRef zip);
bool f_zip_entry_close(CObjRef zip_entry);
int64 f_zip_entry_compressedsize(CObjRef zip_entry);
String f_zip_entry_compressionmethod(CObjRef zip_entry);
int64 f_zip_entry_filesize(CObjRef zip_entry);
String f_zip_entry_name(CObjRef zip_entry);
bool f_zip_entry_open(CObjRef zip, CObjRef zip_entry, CStrRef mode = null_string);
Variant f_zip_entry_read(CObjRef zip_entry, int64 length = 1024);
Variant f_zip_open(CStrRef filename);
Variant f_zip_read(CObjRef zip);
extern const int64 q_ziparchive_CREATE;
extern const int64 q_ziparchive_EXCL;
extern const int64 q_ziparchive_CHECKCONS;
extern const int64 q_ziparchive_OVERWRITE;
extern const int64 q_ziparchive_FL_NOCASE;
extern const int64 q_ziparchive_FL_NODIR;
extern const int64 q_ziparchive_FL_COMPRESSED;
extern const int64 q_ziparchive_FL_UNCHANGED;
extern const int64 q_ziparchive_CM_DEFAULT;
extern const int64 q_ziparchive_CM_STORE;
extern const int64 q_ziparchive_CM_SHRINK;
extern const int64 q_ziparchive_CM_REDUCE_1;
extern const int64 q_ziparchive_CM_REDUCE_2;
extern const int64 q_ziparchive_CM_REDUCE_3;
extern const int64 q_ziparchive_CM_REDUCE_4;
extern const int64 q_ziparchive_CM_IMPLODE;
extern const int64 q_ziparchive_CM_DEFLATE;
extern const int64 q_ziparchive_CM_DEFLATE64;
extern const int64 q_ziparchive_CM_PKWARE_IMPLODE;
extern const int64 q_ziparchive_ER_OK;
extern const int64 q_ziparchive_ER_MULTIDISK;
extern const int64 q_ziparchive_ER_RENAME;
extern const int64 q_ziparchive_ER_CLOSE;
extern const int64 q_ziparchive_ER_SEEK;
extern const int64 q_ziparchive_ER_READ;
extern const int64 q_ziparchive_ER_WRITE;
extern const int64 q_ziparchive_ER_CRC;
extern const int64 q_ziparchive_ER_ZIPCLOSED;
extern const int64 q_ziparchive_ER_NOENT;
extern const int64 q_ziparchive_ER_EXISTS;
extern const int64 q_ziparchive_ER_OPEN;
extern const int64 q_ziparchive_ER_TMPOPEN;
extern const int64 q_ziparchive_ER_ZLIB;
extern const int64 q_ziparchive_ER_MEMORY;
extern const int64 q_ziparchive_ER_CHANGED;
extern const int64 q_ziparchive_ER_COMPNOTSUPP;
extern const int64 q_ziparchive_ER_EOF;
extern const int64 q_ziparchive_ER_INVAL;
extern const int64 q_ziparchive_ER_NOZIP;
extern const int64 q_ziparchive_ER_INTERNAL;
extern const int64 q_ziparchive_ER_INCONS;
extern const int64 q_ziparchive_ER_REMOVE;
extern const int64 q_ziparchive_ER_DELETED;

///////////////////////////////////////////////////////////////////////////////
// class ZipArchive

FORWARD_DECLARE_CLASS(ziparchive);
class c_ziparchive : public ExtObjectData, public Sweepable {
 public:
  BEGIN_CLASS_MAP(ziparchive)
  END_CLASS_MAP(ziparchive)
  DECLARE_CLASS(ziparchive, ZipArchive, ObjectData)
  DECLARE_INVOKES_FROM_EVAL
  ObjectData* dynCreate(CArrRef params, bool init = true);

  // properties
  public: int m_status;
  public: int m_statusSys;
  public: int m_numFiles;
  public: String m_filename;
  public: String m_comment;

  // need to implement
  public: c_ziparchive();
  public: ~c_ziparchive();
  public: void t___construct();
  public: bool t_addemptydir(CStrRef dirname);
  public: bool t_addfile(CStrRef filename, CStrRef localname = null_string);
  public: bool t_addfromstring(CStrRef localname, CStrRef contents);
  public: bool t_close();
  public: bool t_deleteindex(int64 index);
  public: bool t_deletename(CStrRef name);
  public: bool t_extractto(CStrRef destination, CVarRef entries = null_variant);
  public: Variant t_getarchivecomment();
  public: Variant t_getcommentindex(int64 index, int64 flags = 0);
  public: Variant t_getcommentname(CStrRef name, int64 flags = 0);
  public: Variant t_getfromindex(int64 index, int64 flags = 0);
  public: Variant t_getfromname(CStrRef name, int64 flags = 0);
  public: Variant t_getnameindex(int64 index);
  public: Variant t_getstatusstring();
  public: Variant t_getstream(CStrRef name);
  public: Variant t_locatename(CStrRef name, int64 flags = 0);
  public: Variant t_open(CStrRef filename, int64 flags = 0);
  public: bool t_renameindex(int64 index, CStrRef newname);
  public: bool t_renamename(CStrRef name, CStrRef newname);
  public: Variant t_setarchivecomment(CStrRef comment);
  public: Variant t_setcommentindex(int64 index, CStrRef comment);
  public: Variant t_setcommentname(CStrRef name, CStrRef comment);
  public: Variant t_statindex(int64 index, int64 flags = 0);
  public: Variant t_statname(CStrRef name, int64 flags = 0);
  public: Variant t_unchangeall();
  public: Variant t_unchangearchive();
  public: Variant t_unchangeindex(int64 index);
  public: Variant t_unchangename(CStrRef name);
  public: Variant t___destruct();

  // implemented by HPHP
  public: c_ziparchive *create();
  public: void dynConstruct(CArrRef Params);
  public: void dynConstructFromEval(Eval::VariableEnvironment &env,
                                    const Eval::FunctionCallExpression *call);
  public: virtual void destruct();

  private: 
    struct zip *m_archive;
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // __EXT_ZIP_H__
