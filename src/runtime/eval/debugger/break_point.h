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

#ifndef __HPHP_EVAL_DEBUGGER_BREAK_POINT_H__
#define __HPHP_EVAL_DEBUGGER_BREAK_POINT_H__

#include <runtime/eval/debugger/debugger_thrift_buffer.h>
#include <runtime/base/frame_injection.h>

namespace HPHP { namespace Eval {
///////////////////////////////////////////////////////////////////////////////

enum InterruptType {
  SessionStarted,
  SessionEnded,
  RequestStarted,
  RequestEnded,
  PSPEnded,
  BreakPointReached,
  ExceptionThrown,
};

class InterruptSite {
public:
  InterruptSite(FrameInjection *frame, CObjRef e = Object())
      : m_frame(frame), m_exception(e), m_function(NULL), m_file_strlen(-1) {
    ASSERT(m_frame);
  }

  FrameInjection *getFrame() const { return m_frame;}
  const char *getFile() const;
  int getLine() const { return m_frame->getLine();}
  Object getException() { return m_exception;}
  const char *getNamespace() const { return NULL;}
  const char *getClass() const { return m_frame->getClass();}
  const char *getFunction() const;
  int getFileLen() const;

  std::string &url() const { return m_url;}

private:
  FrameInjection *m_frame;
  Object m_exception;

  // cached
  mutable String m_file;
  mutable const char *m_function;
  mutable int m_file_strlen;
  mutable std::string m_url;
};

///////////////////////////////////////////////////////////////////////////////

DECLARE_BOOST_TYPES(BreakPointInfo);
class BreakPointInfo {
public:
  enum State {
    Always   = -1,
    Once     = 1,
    Disabled = 0,
  };

  static const char *GetInterruptName(InterruptType interrupt);

public:
  BreakPointInfo() {} // for thrift
  BreakPointInfo(bool regex, State state, const std::string &file, int line);
  BreakPointInfo(bool regex, State state, InterruptType interrupt,
                 const std::string &url);
  BreakPointInfo(bool regex, State state, InterruptType interrupt,
                 const std::string &exp, const std::string &file);

  void setClause(const std::string &clause, bool check);
  void toggle();

  bool valid();
  bool same(BreakPointInfoPtr bpi);
  bool match(InterruptType interrupt, InterruptSite &site);

  std::string state(bool padding) const;
  std::string desc() const;
  std::string site() const;

  void sendImpl(DebuggerThriftBuffer &thrift);
  void recvImpl(DebuggerThriftBuffer &thrift);

  static void SendImpl(const BreakPointInfoPtrVec &bps,
                       DebuggerThriftBuffer &thrift);
  static void RecvImpl(BreakPointInfoPtrVec &bps,
                       DebuggerThriftBuffer &thrift);

  int8 m_state;
  bool m_valid;
  int8 m_interrupt;

  // file::line1-line2
  std::string m_file;
  int32 m_line1;
  int32 m_line2;

  // class::func()
  std::string m_namespace;
  std::string m_class;
  std::string m_function;

  // URL
  std::string m_url;

  // whether strings are regex
  bool m_regex;

  // "if", "&&" clause
  bool m_check;
  std::string m_clause;
  std::string m_php; // cached

  // server results
  std::string m_output;
  std::string m_exceptionClass;
  std::string m_exceptionObject;

private:
  static bool Match(const char *haystack, int haystack_len,
                    const std::string &needle, bool regex, bool exact);

  std::string regex(const std::string &name) const;

  std::string descBreakPointReached() const;
  std::string descExceptionThrown() const;

  void parseExceptionThrown(const std::string &exp);
  void parseBreakPointReached(const std::string &exp, const std::string &file);
  bool parseLines(const std::string &token);

  bool checkException(CObjRef e);
  bool checkUrl(std::string &url);
  bool checkLines(int line);
  bool checkFrame(FrameInjection *frame);
  bool checkClause();
};

///////////////////////////////////////////////////////////////////////////////
}}

#endif // __HPHP_EVAL_DEBUGGER_BREAK_POINT_H__