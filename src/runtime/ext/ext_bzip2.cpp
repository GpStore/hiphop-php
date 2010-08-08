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

#include <runtime/ext/ext_bzip2.h>
#include <runtime/base/file/bzip2_file.h>

namespace HPHP {
IMPLEMENT_DEFAULT_EXTENSION(bz2);
///////////////////////////////////////////////////////////////////////////////

Variant f_bzcompress(CStrRef source, int blocksize /* = 4 */, int workfactor /* = 0 */) {
  char *dest = NULL;   
  int error; 
  unsigned int source_len, dest_len;  
  
  source_len = source.length();
  dest_len = source.length() + (0.01*source.length()) + 600;
  
  if (!(dest = (char *)malloc(dest_len + 1)))
    return BZ_MEM_ERROR;
    
  error = BZ2_bzBuffToBuffCompress(dest, &dest_len, (char *) source.c_str(), source_len, blocksize, 0, workfactor);
  if (error != BZ_OK) {
    if (dest)
      free(dest);
    return error;
  } else {
    String ret = String(dest, dest_len, CopyString);
    free(dest);
    return ret;
  }
}

Variant f_bzdecompress(CStrRef source, int small /* = 0 */) {
  char *dest;
  int source_len = source.length();
  int error;
  uint64 size = 0;
  bz_stream bzs;

  bzs.bzalloc = NULL;
  bzs.bzfree = NULL;

  if (BZ2_bzDecompressInit(&bzs, 0, small) != BZ_OK) {
    return false;
  }

  bzs.next_in = (char *) source.c_str();
  bzs.avail_in = source_len;

  /* in most cases bz2 offers at least 2:1 compression, so we use that as our base */
  bzs.avail_out = source_len * 2;
  bzs.next_out = dest = (char *) malloc(bzs.avail_out + 1);
  if (!dest)
    return BZ_MEM_ERROR;
  
  while ((error = BZ2_bzDecompress(&bzs)) == BZ_OK && bzs.avail_in > 0) {
    /* compression is better then 2:1, need to allocate more memory */
    bzs.avail_out = source_len;
    size = (bzs.total_out_hi32 * (unsigned int) -1) + bzs.total_out_lo32;
    dest = (char *) realloc(dest, bzs.avail_out+1);
    if (!dest)
      return BZ_MEM_ERROR;
    bzs.next_out = dest + size;
  }

  if (error == BZ_STREAM_END || error == BZ_OK) {
    size = (bzs.total_out_hi32 * (unsigned int) -1) + bzs.total_out_lo32;
    dest = (char *) realloc(dest, size);
    if (!dest)
      return BZ_MEM_ERROR;
    String ret = String(dest, size, CopyString);
    free(dest);
    BZ2_bzDecompressEnd(&bzs);
    return ret;
  } else { /* real error */
    if (dest)
      free(dest);
    BZ2_bzDecompressEnd(&bzs);
    return error;
  }
}

int f_bzerrno(CObjRef bz) {
  BZ2File *f = bz.getTyped<BZ2File>();
  return f->errnu();
}

Variant f_bzerror(CObjRef bz) {
  BZ2File *f = bz.getTyped<BZ2File>();
  return f->error();
}

String f_bzerrstr(CObjRef bz) {
  BZ2File *f = bz.getTyped<BZ2File>();
  return f->errstr();
}

Variant f_bzflush(CObjRef bz) {
  BZ2File *f = bz.getTyped<BZ2File>();
  return f->flush();
}

Variant f_bzopen(CStrRef filename, CStrRef mode) {
  if (mode != "r" && mode != "w")
    return false;
  BZ2File *bz = NEW(BZ2File)();
  Object handle(bz);
  bool ret = bz->open(File::TranslatePath(filename), mode);
  if (!ret) {
    raise_warning("%s",Util::safe_strerror(errno).c_str());
    return false;
  }
  return handle;
}

///////////////////////////////////////////////////////////////////////////////
}

