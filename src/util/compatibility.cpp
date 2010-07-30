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

#include "compatibility.h"

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////
#ifdef DARWIN
char *strndup (char const *source, size_t length) {
  size_t len = strlen(source);
  len = (len>length)?length:len;
  char *dest;
  if (!(dest=(char *)malloc(len+1))) return NULL;
  dest[len] = '\0';
  return (char *)memcpy(dest, source, len);
}
#endif


///////////////////////////////////////////////////////////////////////////////
}
