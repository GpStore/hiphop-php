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

#include <test/test_ext_zip.h>
#include <runtime/ext/ext_zip.h>

IMPLEMENT_SEP_EXTENSION_TEST(Zip);
///////////////////////////////////////////////////////////////////////////////

bool TestExtZip::RunTests(const std::string &which) {
  bool ret = true;

  RUN_TEST(test_zip_close);
  RUN_TEST(test_zip_entry_close);
  RUN_TEST(test_zip_entry_compressedsize);
  RUN_TEST(test_zip_entry_compressionmethod);
  RUN_TEST(test_zip_entry_filesize);
  RUN_TEST(test_zip_entry_name);
  RUN_TEST(test_zip_entry_open);
  RUN_TEST(test_zip_entry_read);
  RUN_TEST(test_zip_open);
  RUN_TEST(test_zip_read);
//  RUN_TEST(test_ZipArchive);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////

bool TestExtZip::test_zip_close() {
  // tested in zip_entry_read
  return Count(true);
}

bool TestExtZip::test_zip_entry_close() {
  // tested in zip_entry_read
  return Count(true);
}

bool TestExtZip::test_zip_entry_compressedsize() {
  // tested in zip_entry_read
  return Count(true);
}

bool TestExtZip::test_zip_entry_compressionmethod() {
  // tested in zip_entry_read
  return Count(true);
}

bool TestExtZip::test_zip_entry_filesize() {
  // tested in zip_entry_read
  return Count(true);
}

bool TestExtZip::test_zip_entry_name() {
  // tested in zip_entry_read
  return Count(true);
}

bool TestExtZip::test_zip_entry_open() {
  // tested in zip_entry_read
  return Count(true);
}

bool TestExtZip::test_zip_entry_read() {
  Variant zip = f_zip_open("test/test_procedural.zip");
  Variant entry = f_zip_read(zip);
  VERIFY(f_zip_entry_open(zip, entry)); 
  VS(f_zip_entry_name(entry), "foo"); 
  VS(f_zip_entry_compressionmethod(entry), "stored"); 
  VS(f_zip_entry_compressedsize(entry), 5); 
  VS(f_zip_entry_filesize(entry), 5); 
  VS(f_zip_entry_read(entry), "foo\n\n");
  VERIFY(f_zip_entry_close(entry));
  f_zip_close(zip);
  return Count(true);
}

bool TestExtZip::test_zip_open() {
  // tested in zip_entry_read
  return Count(true);
}

bool TestExtZip::test_zip_read() {
  // tested in zip_entry_read
  return Count(true);
}
