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

#ifndef __HPHP_THRIFT_BUFFER_H__
#define __HPHP_THRIFT_BUFFER_H__

#include <runtime/base/types.h>
#include <runtime/base/complex_types.h>

#include <arpa/inet.h>
#ifdef FREEBSD
#include <sys/endian.h>
#else
#include <byteswap.h>
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#ifdef FREEBSD
#define htonll(x) bswap64(x)
#define ntohll(x) bswap64(x)
#else
#define htonll(x) bswap_64(x)
#define ntohll(x) bswap_64(x)
#endif
#else
#define htonll(x) (x)
#define ntohll(x) (x)
#endif

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

/**
 * Efficient thrift input/output preparation. Used by automatically generated
 * separable extension code created by running thrift compiler, for example,
 *
 *   thrift --gen hphp my_service.thrift
 */
class ThriftBuffer {
public:
  enum TType {
    T_STOP   = 0,
    T_VOID   = 1,
    T_BOOL   = 2,
    T_BYTE   = 3,
    T_I08    = 3,
    T_DOUBLE = 4,
    T_I16    = 6,
    T_I32    = 8,
    T_U64    = 9,
    T_I64    = 10,
    T_STRING = 11,
    T_UTF7   = 11,
    T_STRUCT = 12,
    T_MAP    = 13,
    T_SET    = 14,
    T_LIST   = 15,
    T_UTF8   = 16,
    T_UTF16  = 17,
  };

  enum TMessageType {
    CALL  = 1,
    REPLY = 2,
    EXCEPTION = 3,
  };

public:
  /**
   * Constructing with some initial size, subsequent allocation will double
   * existing size every round.
   */
  ThriftBuffer();
  ThriftBuffer(int); // only for hphp/main.cpp to load this class
  ~ThriftBuffer();

  // passing in input and output transport objects
  void create(CObjRef xin, CObjRef xout) {
    m_xin = xin;
    m_xout = xout;
  }

  void flush(); // write bytes to transport
  void reset(bool read); // get ready for reads or writes

  // input has been called with htons() call
  void nwrite(int16 data) {
    memcpy(m_p, &data, sizeof(data));
    if ((m_p += sizeof(data)) > m_pSafe) flush();
  }

  // input has not been called with hton() calls yet
  void write(bool data) {
    *m_p = (data ? 1 : 0);
    if (++m_p > m_pSafe) flush();
  }
  void write(int8 data) {
    *m_p = data;
    if (++m_p > m_pSafe) flush();
  }
  void write(int16 data) {
    data = htons(data);
    memcpy(m_p, &data, sizeof(data));
    if ((m_p += sizeof(data)) > m_pSafe) flush();
  }
  void write(int32 data) {
    data = htonl(data);
    memcpy(m_p, &data, sizeof(data));
    if ((m_p += sizeof(data)) > m_pSafe) flush();
  }
  void write(int64 data) {
    data = htonll(data);
    memcpy(m_p, &data, sizeof(data));
    if ((m_p += sizeof(data)) > m_pSafe) flush();
  }
  void write(double data) {
    union { int64 c; double d;} a;
    a.d = data;
    write(a.c);
  }
  void write(CStrRef data);

  // reads
  void read(bool &data) {
    if (m_safe) {
      data = *m_p;
      if (++m_p > m_pSafe) m_safe = false;
    } else {
      read((char*)&data, (int)sizeof(data));
    }
  }
  void read(int8 &data) {
    if (m_safe) {
      data = *m_p;
      if (++m_p > m_pSafe) m_safe = false;
    } else {
      read((char*)&data, (int)sizeof(data));
    }
  }
  void read(int16 &data) {
    if (m_safe) {
      memcpy(&data, m_p, sizeof(data));
      if ((m_p += sizeof(data)) > m_pSafe) m_safe = false;
    } else {
      read((char*)&data, (int)sizeof(data));
    }
    data = ntohs(data);
  }
  void read(int32 &data) {
    if (m_safe) {
      memcpy(&data, m_p, sizeof(data));
      if ((m_p += sizeof(data)) > m_pSafe) m_safe = false;
    } else {
      read((char*)&data, (int)sizeof(data));
    }
    data = ntohl(data);
  }
  void read(int64 &data) {
    if (m_safe) {
      memcpy(&data, m_p, sizeof(data));
      if ((m_p += sizeof(data)) > m_pSafe) m_safe = false;
    } else {
      read((char*)&data, (int)sizeof(data));
    }
    data = ntohll(data);
  }
  void read(double &data) {
    union { int64 c; double d;} a; a.d = data;
    read(a.c);
    data = a.d;
  }
  void read(String &data) {
    int32 size;
    read(size);
    if (size > 0 && ++size > 0) {
      char *buf = (char*)malloc(size);
      if (!buf) throwOutOfMemory();
      read(buf, size);
      buf[size] = '\0';
      data = String(buf, size, AttachString);
    } else {
      throwInvalidStringSize(size);
    }
  }

  void skip(int8 type);

private:
  // disabling copy constructor and assignment
  ThriftBuffer(const ThriftBuffer &sb) { ASSERT(false);}
  ThriftBuffer &operator=(const ThriftBuffer &sb) {ASSERT(false);return *this;}

  char *m_p;
  char *m_pSafe;
  char *m_pEnd;
  bool  m_safe;

  char  *m_buf;
  Object m_xin;
  Object m_xout;

  void flush(CStrRef data);
  void read(char *data, int len);

  void throwOutOfMemory();
  void throwInvalidStringSize(int size);
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // __HPHP_THRIFT_BUFFER_H__
