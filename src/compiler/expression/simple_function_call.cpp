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

#include <compiler/expression/simple_function_call.h>
#include <compiler/analysis/dependency_graph.h>
#include <compiler/analysis/file_scope.h>
#include <compiler/analysis/function_scope.h>
#include <compiler/analysis/class_scope.h>
#include <compiler/analysis/code_error.h>
#include <compiler/expression/expression_list.h>
#include <compiler/statement/statement_list.h>
#include <compiler/statement/exp_statement.h>
#include <compiler/statement/return_statement.h>
#include <compiler/statement/method_statement.h>
#include <compiler/expression/scalar_expression.h>
#include <compiler/expression/constant_expression.h>
#include <compiler/expression/parameter_expression.h>
#include <compiler/expression/assignment_expression.h>
#include <compiler/expression/unary_op_expression.h>
#include <compiler/analysis/constant_table.h>
#include <compiler/analysis/variable_table.h>
#include <util/util.h>
#include <compiler/option.h>
#include <compiler/expression/simple_variable.h>
#include <compiler/parser/parser.h>
#include <compiler/parser/hphp.tab.hpp>
#include <runtime/base/complex_types.h>
#include <runtime/base/externals.h>
#include <runtime/base/execution_context.h>

using namespace HPHP;
using namespace std;
using namespace boost;

///////////////////////////////////////////////////////////////////////////////
// statics

std::map<std::string, int>SimpleFunctionCall::FunctionTypeMap;

#define CHECK_HOOK(n)                                   \
  (SimpleFunctionCall::m_hookHandler ?                  \
   SimpleFunctionCall::m_hookHandler(ar, this, n) : 0)

Expression *(*SimpleFunctionCall::m_hookHandler)
  (AnalysisResultPtr ar, SimpleFunctionCall *call, HphpHookUniqueId id);

void SimpleFunctionCall::InitFunctionTypeMap() {
  if (FunctionTypeMap.empty()) {
    FunctionTypeMap["define"]               = DefineFunction;
    FunctionTypeMap["create_function"]      = CreateFunction;

    FunctionTypeMap["func_get_arg"]         = VariableArgumentFunction;
    FunctionTypeMap["func_get_args"]        = VariableArgumentFunction;
    FunctionTypeMap["func_num_args"]        = VariableArgumentFunction;

    FunctionTypeMap["extract"]              = ExtractFunction;
    FunctionTypeMap["compact"]              = CompactFunction;

    FunctionTypeMap["shell_exec"]           = ShellExecFunction;
    FunctionTypeMap["exec"]                 = ShellExecFunction;
    FunctionTypeMap["passthru"]             = ShellExecFunction;
    FunctionTypeMap["system"]               = ShellExecFunction;

    FunctionTypeMap["defined"]              = DefinedFunction;
    FunctionTypeMap["function_exists"]      = FunctionExistsFunction;
    FunctionTypeMap["class_exists"]         = ClassExistsFunction;
    FunctionTypeMap["interface_exists"]     = InterfaceExistsFunction;
    FunctionTypeMap["constant"]             = ConstantFunction;

    FunctionTypeMap["unserialize"]          = UnserializeFunction;

    FunctionTypeMap["get_defined_vars"]     = GetDefinedVarsFunction;
  }
}

///////////////////////////////////////////////////////////////////////////////
// constructors/destructors

SimpleFunctionCall::SimpleFunctionCall
(EXPRESSION_CONSTRUCTOR_PARAMETERS,
 const std::string &name, ExpressionListPtr params, ExpressionPtr cls)
  : FunctionCall(EXPRESSION_CONSTRUCTOR_PARAMETER_VALUES,
                 ExpressionPtr(), name, params, cls),
    m_type(UnknownType), m_dynamicConstant(false),
    m_parentClass(false), m_builtinFunction(false), m_noPrefix(false),
    m_invokeFewArgsDecision(true),
    m_dynamicInvoke(false),
    m_hookData(NULL) {

  m_dynamicInvoke = Option::DynamicInvokeFunctions.find(m_name) !=
    Option::DynamicInvokeFunctions.end();

  if (!m_class && m_className.empty()) {
    if (FunctionTypeMap.empty()) InitFunctionTypeMap();
    map<string, int>::const_iterator iter =
      FunctionTypeMap.find(m_name);
    if (iter != FunctionTypeMap.end()) {
      m_type = iter->second;
    }
  }
}

SimpleFunctionCall::~SimpleFunctionCall() {
  if (m_hookData) {
    ASSERT(m_hookHandler);
    m_hookHandler(AnalysisResultPtr(), this, hphpUniqueDtor);
  }
}

ExpressionPtr SimpleFunctionCall::clone() {
  SimpleFunctionCallPtr exp(new SimpleFunctionCall(*this));
  FunctionCall::deepCopy(exp);
  exp->m_hookData = 0;
  return exp;
}

///////////////////////////////////////////////////////////////////////////////
// parser functions

void SimpleFunctionCall::onParse(AnalysisResultPtr ar) {
  if (m_class) return;

  FileScopePtr fs = ar->getFileScope();
  ConstructPtr self = shared_from_this();
  if (m_className.empty()) {
    CodeErrorPtr codeError = ar->getCodeError();
    switch (m_type) {
    case CreateFunction:
      if (m_params->getCount() == 2 &&
          (*m_params)[0]->isLiteralString() &&
          (*m_params)[1]->isLiteralString()) {
        FunctionScopePtr func = ar->getFunctionScope();
        if (func) func->disableInline();
        string params = (*m_params)[0]->getLiteralString();
        string body = (*m_params)[1]->getLiteralString();
        m_lambda = CodeGenerator::GetNewLambda();
        string code = "function " + m_lambda + "(" + params + ") "
          "{" + body + "}";
        ar->appendExtraCode(code);
      }
      break;
    case VariableArgumentFunction:
      /*
        Note:
        At this point, we dont have a function scope, so we set
        the flags on the FileScope.
        The FileScope maintains a stack of attributes, so that
        it correctly handles each function.
        But note that later phases should set/get the attribute
        directly on the FunctionScope, rather than on the FileScope
      */
      ar->getFileScope()->setAttribute(FileScope::VariableArgument);
      break;
    case ExtractFunction:
      ar->getCodeError()->record(self, CodeError::UseExtract, self);
      ar->getFileScope()->setAttribute(FileScope::ContainsLDynamicVariable);
      ar->getFileScope()->setAttribute(FileScope::ContainsExtract);
      break;
    case CompactFunction:
      ar->getFileScope()->setAttribute(FileScope::ContainsDynamicVariable);
      ar->getFileScope()->setAttribute(FileScope::ContainsCompact);
      break;
    case ShellExecFunction:
      ar->getCodeError()->record(self, CodeError::UseShellExec, self);
      break;
    case GetDefinedVarsFunction:
      ar->getFileScope()->setAttribute(FileScope::ContainsDynamicVariable);
      ar->getFileScope()->setAttribute(FileScope::ContainsGetDefinedVars);
      ar->getFileScope()->setAttribute(FileScope::ContainsCompact);
      break;
    default:
      CHECK_HOOK(onSimpleFunctionCallFuncType);
      break;
    }
  }

  string call = getText();
  string name = m_name;
  if (!m_className.empty()) {
    name = m_className + "::" + name;
  }
  ar->getDependencyGraph()->add(DependencyGraph::KindOfFunctionCall, call,
                                shared_from_this(), name);
}

///////////////////////////////////////////////////////////////////////////////
// static analysis functions

void SimpleFunctionCall::addDependencies(AnalysisResultPtr ar) {
  if (!m_class) {
    if (m_className.empty()) {
      addUserFunction(ar, m_name);
    } else if (m_origClassName != "parent") {
      addUserClass(ar, m_className);
    } else {
      m_parentClass = true;
    }
  }
}

void SimpleFunctionCall::analyzeProgram(AnalysisResultPtr ar) {
  if (m_class) {
    m_class->analyzeProgram(ar);
    setDynamicByIdentifier(ar, m_name);
  } else {
    addDependencies(ar);
  }

  if (ar->getPhase() == AnalysisResult::AnalyzeInclude) {

    CHECK_HOOK(onSimpleFunctionCallAnalyzeInclude);

    ConstructPtr self = shared_from_this();

    // We need to know the name of the constant so that we can associate it
    // with this file before we do type inference.
    if (!m_class && m_className.empty() && m_type == DefineFunction &&
        m_params && m_params->getCount() >= 2) {
      ExpressionPtr ename = (*m_params)[0];
      if (ConstantExpressionPtr cname =
          dynamic_pointer_cast<ConstantExpression>(ename)) {

        ename = ExpressionPtr(
          new ScalarExpression(cname->getLocation(), KindOfScalarExpression,
                               T_STRING, cname->getName(), true));
        m_params->removeElement(0);
        m_params->insertElement(ename);
      }
      ScalarExpressionPtr name =
        dynamic_pointer_cast<ScalarExpression>(ename);
      if (name) {
        string varName = name->getIdentifier();
        if (!varName.empty()) {
          ar->getFileScope()->declareConstant(ar, varName);

          // handling define("CONSTANT", ...);
          ExpressionPtr value = (*m_params)[1];
          BlockScopePtr block = ar->findConstantDeclarer(varName);
          ConstantTablePtr constants = block->getConstants();
          if (constants != ar->getConstants()) {
            constants->add(varName, NEW_TYPE(Some), value, ar, self);
            if (name->hasHphpNote("Dynamic")) {
              constants->setDynamic(ar, varName);
            }
          }
        }
      }
    }

    if (m_type == UnserializeFunction) {
      ar->forceClassVariants();
    }
  }

  if (ar->getPhase() == AnalysisResult::AnalyzeAll) {
    // Look up the corresponding FunctionScope and ClassScope
    // for this function call
    {
      FunctionScopePtr func;
      ClassScopePtr cls;
      if (!m_class && m_className.empty()) {
        func = ar->findFunction(m_name);
      } else {
        cls = ar->resolveClass(m_className);
        if (cls) {
          if (m_name == "__construct") {
            func = cls->findConstructor(ar, true);
          } else {
            func = cls->findFunction(ar, m_name, true, true);
          }
        }
      }
      if (func && !func->isRedeclaring()) {
        if (m_funcScope != func) {
          m_funcScope = func;
          Construct::recomputeEffects();
        }
      }
      if (cls && !cls->isRedeclaring())
        m_classScope = cls;
    }
    // check for dynamic constant and volatile function/class
    if (!m_class && m_className.empty() &&
      (m_type == DefinedFunction ||
       m_type == FunctionExistsFunction ||
       m_type == ClassExistsFunction ||
       m_type == InterfaceExistsFunction) &&
      m_params && m_params->getCount() >= 1) {
      ExpressionPtr value = (*m_params)[0];
      if (value->isScalar()) {
        ScalarExpressionPtr name =
          dynamic_pointer_cast<ScalarExpression>(value);
        if (name && name->isLiteralString()) {
          string symbol = name->getLiteralString();
          switch (m_type) {
          case DefinedFunction: {
            ConstantTablePtr constants = ar->getConstants();
            if (!constants->isPresent(symbol)) {
              // user constant
              BlockScopePtr block = ar->findConstantDeclarer(symbol);
              if (block) { // found the constant
                constants = block->getConstants();
                // set to be dynamic
                constants->setDynamic(ar, symbol);
              }
            }
            break;
          }
          case FunctionExistsFunction: {
            FunctionScopePtr func = ar->findFunction(Util::toLower(symbol));
            if (func && func->isUserFunction()) {
              func->setVolatile();
            }
            break;
          }
          case InterfaceExistsFunction:
          case ClassExistsFunction: {
            ClassScopePtr cls = ar->findClass(Util::toLower(symbol));
            if (cls && cls->isUserClass()) {
              cls->setVolatile();
            }
            break;
          }
          default:
            ASSERT(false);
          }
        }
      }
    }
  }

  if (m_params) {
    if (ar->getPhase() == AnalysisResult::AnalyzeAll) {
      markRefParams(m_funcScope, m_name, canInvokeFewArgs());
    }

    m_params->analyzeProgram(ar);
  }
}

bool SimpleFunctionCall::isDefineWithoutImpl(AnalysisResultPtr ar) {
  if (m_class || !m_className.empty()) return false;
  if (m_type == DefineFunction && m_params && m_params->getCount() >= 2) {
    if (m_dynamicConstant) return false;
    ScalarExpressionPtr name =
      dynamic_pointer_cast<ScalarExpression>((*m_params)[0]);
    if (!name) return false;
    string varName = name->getIdentifier();
    if (varName.empty()) return false;
    if (ar->getConstants()->isSystem(varName)) return true;
    ExpressionPtr value = (*m_params)[1];
    return (!ar->isConstantRedeclared(varName)) && value->isScalar();
  } else {
    return false;
  }
}

//typedef std::map<std::string, ExpressionPtr> StringToExpressionPtrMap;

static ExpressionPtr cloneForInlineRecur(ExpressionPtr exp,
                                         const std::string &prefix,
                                         StringToExpressionPtrMap &sepm,
                                         AnalysisResultPtr ar) {
  for (int i = 0, n = exp->getKidCount(); i < n; i++) {
    if (ExpressionPtr k = exp->getNthExpr(i)) {
      exp->setNthKid(i, cloneForInlineRecur(k, prefix, sepm, ar));
    }
  }
  switch (exp->getKindOf()) {
  case Expression::KindOfSimpleVariable:
    {
      SimpleVariablePtr sv(dynamic_pointer_cast<SimpleVariable>(exp));
      if (!sv->isThis() && !sv->isSuperGlobal()) {
        string name = prefix + sv->getName();
        ExpressionPtr rep(new SimpleVariable(exp->getLocation(),
                                             exp->getKindOf(), name));
        rep->copyContext(exp);
        sepm[name] = rep;
        exp = rep;
      }
    }
    break;
  case Expression::KindOfSimpleFunctionCall:
    {
      AnalysisResult::Phase phase = ar->getPhase();
      ar->setPhase(AnalysisResult::AnalyzeAll);
      static_pointer_cast<SimpleFunctionCall>(exp)->addDependencies(ar);
      ar->setPhase(phase);
    }
  default:
    break;
  }
  return exp;
}

static ExpressionPtr cloneForInline(ExpressionPtr exp,
                                    const std::string &prefix,
                                    StringToExpressionPtrMap &sepm,
                                    AnalysisResultPtr ar) {
  return cloneForInlineRecur(exp->clone(), prefix, sepm, ar);
}

static int cloneStmtsForInline(ExpressionListPtr elist, StatementPtr s,
                               const std::string &prefix,
                               StringToExpressionPtrMap &sepm,
                               AnalysisResultPtr ar) {
  switch (s->getKindOf()) {
  case Statement::KindOfStatementList:
    {
      for (int i = 0, n = s->getKidCount(); i < n; ++i) {
        if (int ret = cloneStmtsForInline(elist, s->getNthStmt(i),
                                          prefix, sepm, ar)) {
          return ret;
        }
      }
      return 0;
    }
  case Statement::KindOfExpStatement:
    elist->addElement(cloneForInline(dynamic_pointer_cast<ExpStatement>(s)->
                                     getExpression(), prefix, sepm, ar));
    return 0;
  case Statement::KindOfReturnStatement:
    {
      ExpressionPtr exp =
        dynamic_pointer_cast<ReturnStatement>(s)->getRetExp();

      if (exp) {
        elist->addElement(cloneForInline(exp, prefix, sepm, ar));
        return 1;
      }
      return -1;
    }
  default:
    assert(false);
  }
  return 1;
}

ExpressionPtr SimpleFunctionCall::optimize(AnalysisResultPtr ar) {
  if (m_class || !m_className.empty() || !m_funcScope) return ExpressionPtr();

  if (!m_funcScope->isUserFunction()) {
    if (m_type == UnknownType && m_funcScope->isFoldable()) {
      Array arr;
      if (m_params) {
        if (!m_params->isScalar()) return ExpressionPtr();
        for (int i = 0, n = m_params->getCount(); i < n; ++i) {
          Variant v;
          if (!(*m_params)[i]->getScalarValue(v)) return ExpressionPtr();
          arr.set(i, v);
        }
      }
      try {
        g_context->setThrowAllErrors(true);
        Variant v = invoke_builtin(m_funcScope->getName().c_str(),
                                   arr, -1, true);
        g_context->setThrowAllErrors(false);
        return MakeScalarExpression(ar, getLocation(), v);
      } catch (...) {
        g_context->setThrowAllErrors(false);
      }
      return ExpressionPtr();
    }
  }

  if (!m_funcScope->getInlineAsExpr() ||
      !m_funcScope->getStmt() ||
      m_funcScope->getStmt()->getRecursiveCount() > 10) {
    return ExpressionPtr();
  }

  FunctionScopePtr fs = ar->getFunctionScope();
  if (!fs || fs->inPseudoMain()) return ExpressionPtr();
  VariableTablePtr vt = fs->getVariables();
  int nAct = m_params ? m_params->getCount() : 0;
  int nMax = m_funcScope->getMaxParamCount();
  if (unsigned(nAct - m_funcScope->getMinParamCount()) > (unsigned)nMax ||
      vt->getAttribute(VariableTable::ContainsDynamicVariable) ||
      vt->getAttribute(VariableTable::ContainsExtract) ||
      vt->getAttribute(VariableTable::ContainsCompact) ||
      vt->getAttribute(VariableTable::ContainsGetDefinedVars)) {
    return ExpressionPtr();
  }

  ExpressionListPtr elist(new ExpressionList(getLocation(),
                                             KindOfExpressionList,
                                             ExpressionList::ListKindWrapped));

  std::ostringstream oss;
  oss << "inl" << m_funcScope->nextInlineIndex() << "_" << m_name << "_";
  std::string prefix = oss.str();

  MethodStatementPtr m
    (dynamic_pointer_cast<MethodStatement>(m_funcScope->getStmt()));
  ExpressionListPtr plist = m->getParams();

  int i;
  StringToExpressionPtrMap sepm;

  for (i = 0; i < nMax; i++) {
    ParameterExpressionPtr param
      (dynamic_pointer_cast<ParameterExpression>((*plist)[i]));
    ExpressionPtr arg = i < nAct ? (*m_params)[i] :
      param->defaultValue()->clone();
    SimpleVariablePtr var
      (new SimpleVariable((i < nAct ? arg.get() : this)->getLocation(),
                          KindOfSimpleVariable,
                          prefix + param->getName()));
    bool ref = m_funcScope->isRefParam(i);
    AssignmentExpressionPtr ae
      (new AssignmentExpression(arg->getLocation(), KindOfAssignmentExpression,
                                var, arg, ref));
    elist->addElement(ae);
    if (i < nAct && (ref || !arg->isScalar())) {
      sepm[var->getName()] = var;
    }
  }

  if (cloneStmtsForInline(elist, m->getStmts(), prefix, sepm, ar) <= 0) {
    elist->addElement(CONSTANT("null"));
  }

  if (sepm.size()) {
    ExpressionListPtr unset_list
      (new ExpressionList(this->getLocation(), KindOfExpressionList));

    for (StringToExpressionPtrMap::iterator it = sepm.begin(), end = sepm.end();
         it != end; ++it) {
      ExpressionPtr var = it->second->clone();
      var->clearContext((Context)(unsigned)-1);
      unset_list->addElement(var);
    }

    ExpressionPtr unset(
      new UnaryOpExpression(this->getLocation(), KindOfUnaryOpExpression,
                            unset_list, T_UNSET, true));
    i = elist->getCount();
    ExpressionPtr ret = (*elist)[--i];
    if (ret->isScalar()) {
      elist->insertElement(unset, i);
    } else {
      ExpressionListPtr result_list
        (new ExpressionList(this->getLocation(), KindOfExpressionList,
                            ExpressionList::ListKindLeft));
      result_list->addElement(ret);
      result_list->addElement(unset);
      (*elist)[i] = result_list;
    }
  }

  elist->copyContext(static_pointer_cast<Expression>(shared_from_this()));
  return elist;
}

ExpressionPtr SimpleFunctionCall::preOptimize(AnalysisResultPtr ar) {
  if (m_class) {
    ar->preOptimize(m_class);
    updateClassName();
  }
  ar->preOptimize(m_nameExp);
  ar->preOptimize(m_params);
  if (ar->getPhase() >= AnalysisResult::FirstPreOptimize) {
    if (Expression *rep = CHECK_HOOK(onSimpleFunctionCallPreOptimize)) {
      ExpressionPtr tmp(rep);
      ar->preOptimize(tmp);
      return tmp;
    }
  }

  if (ar->getPhase() != AnalysisResult::SecondPreOptimize) {
    return ExpressionPtr();
  }
  if (ExpressionPtr rep = optimize(ar)) {
    return rep;
  }
  // optimize away various "exists" functions, this may trigger
  // dead code elimination and improve type-inference.
  if (!m_class && m_className.empty() &&
      (m_type == DefinedFunction ||
       m_type == FunctionExistsFunction ||
       m_type == ClassExistsFunction ||
       m_type == InterfaceExistsFunction) &&
      m_params && m_params->getCount() == 1) {
    ExpressionPtr value = (*m_params)[0];
    if (value->isScalar()) {
      ScalarExpressionPtr name = dynamic_pointer_cast<ScalarExpression>(value);
      if (name && name->isLiteralString()) {
        string symbol = name->getLiteralString();
        switch (m_type) {
        case DefinedFunction: {
          ConstantTablePtr constants = ar->getConstants();
          // system constant
          if (constants->isPresent(symbol)) {
            return CONSTANT("true");
          }
          // user constant
          BlockScopePtr block = ar->findConstantDeclarer(symbol);
          // not found (i.e., undefined)
          if (!block) {
            if (symbol.find("::") == std::string::npos) {
              return CONSTANT("false");
            } else {
              // e.g., defined("self::ZERO")
              return ExpressionPtr();
            }
          }
          constants = block->getConstants();
          // already set to be dynamic
          if (constants->isDynamic(symbol)) return ExpressionPtr();
          ConstructPtr decl = constants->getValue(symbol);
          ExpressionPtr constValue = dynamic_pointer_cast<Expression>(decl);
          if (constValue->isScalar()) {
            return CONSTANT("true");
          } else {
            return ExpressionPtr();
          }
          break;
        }
        case FunctionExistsFunction: {
          const std::string &lname = Util::toLower(symbol);
          if (Option::DynamicInvokeFunctions.find(lname) ==
              Option::DynamicInvokeFunctions.end()) {
            FunctionScopePtr func = ar->findFunction(lname);
            if (!func) {
              return CONSTANT("false");
            } else if (!func->isVolatile()) {
              return CONSTANT("true");
            }
          }
          break;
        }
        case InterfaceExistsFunction: {
          ClassScopePtrVec classes = ar->findClasses(Util::toLower(symbol));
          bool interfaceFound = false;
          for (ClassScopePtrVec::const_iterator it = classes.begin();
               it != classes.end(); ++it) {
            ClassScopePtr cls = *it;
            if (cls->isInterface()) {
              interfaceFound = true;
              break;
            }
          }
          if (!interfaceFound) return CONSTANT("false");
          if (classes.size() == 1 && !classes.back()->isVolatile()) {
            return CONSTANT("true");
          }
          break;
        }
        case ClassExistsFunction: {
          ClassScopePtrVec classes = ar->findClasses(Util::toLower(symbol));
          bool classFound = false;
          for (ClassScopePtrVec::const_iterator it = classes.begin();
               it != classes.end(); ++it) {
            ClassScopePtr cls = *it;
            if (!cls->isInterface()) {
              classFound = true;
              break;
            }
          }
          if (!classFound) return CONSTANT("false");
          if (classes.size() == 1 && !classes.back()->isVolatile()) {
            return CONSTANT("true");
          }
          break;
        }
        default:
          ASSERT(false);
        }
      }
    }
  }
  return ExpressionPtr();
}

ExpressionPtr SimpleFunctionCall::postOptimize(AnalysisResultPtr ar) {
  if (!Option::KeepStatementsWithNoEffect && isDefineWithoutImpl(ar)) {
    Construct::recomputeEffects();
    return CONSTANT("true");
  }
  /*
    Dont do this for now. Need to take account of newly created
    variables etc (which would normally be handled by inferTypes).

    if (ExpressionPtr rep = optimize(ar)) {
      return rep;
    }
  */
  return FunctionCall::postOptimize(ar);
}

int SimpleFunctionCall::getLocalEffects() const {
  if (m_class) return UnknownEffect;

  if (m_funcScope && !m_funcScope->hasEffect()) {
    return 0;
  }

  return UnknownEffect;
}

TypePtr SimpleFunctionCall::inferTypes(AnalysisResultPtr ar, TypePtr type,
                                       bool coerce) {
  ASSERT(false);
  return TypePtr();
}

TypePtr SimpleFunctionCall::inferAndCheck(AnalysisResultPtr ar, TypePtr type,
                                          bool coerce) {
  reset();

  if (m_class) {
    m_class->inferAndCheck(ar, NEW_TYPE(Any), false);
  }

  ConstructPtr self = shared_from_this();

  // handling define("CONSTANT", ...);
  if (!m_class && m_className.empty()) {
    if (m_type == DefineFunction && m_params && m_params->getCount() >= 2) {
      ScalarExpressionPtr name =
        dynamic_pointer_cast<ScalarExpression>((*m_params)[0]);
      string varName;
      if (name) {
        varName = name->getIdentifier();
        if (!varName.empty()) {
          ExpressionPtr value = (*m_params)[1];
          TypePtr varType = value->inferAndCheck(ar, NEW_TYPE(Some), false);
          ar->getDependencyGraph()->
            addParent(DependencyGraph::KindOfConstant,
                      ar->getName(), varName, self);
          BlockScopePtr block = ar->findConstantDeclarer(varName);
          if (!block) {
            ar->getFileScope()->declareConstant(ar, varName);
            block = ar->findConstantDeclarer(varName);
            ASSERT(block);
          }
          ConstantTablePtr constants = block->getConstants();
          if (constants != ar->getConstants()) {
            if (value && !value->isScalar()) {
              constants->setDynamic(ar, varName);
              varType = Type::Variant;
            }
            if (constants->isDynamic(varName)) {
              m_dynamicConstant = true;
              ar->getScope()->getVariables()->
                setAttribute(VariableTable::NeedGlobalPointer);
            } else {
              constants->setType(ar, varName, varType, true);
            }
            // in case the old 'value' has been optimized
            constants->setValue(ar, varName, value);
          }
          return checkTypesImpl(ar, type, Type::Boolean, coerce);
        }
      }
      if (varName.empty() && ar->isFirstPass()) {
        ar->getCodeError()->record(self, CodeError::BadDefine, self);
      }
    } else if (m_type == ExtractFunction) {
      ar->getScope()->getVariables()->forceVariants(ar);
    }
  }

  FunctionScopePtr func;

  // avoid raising both MissingObjectContext and UnknownFunction
  bool errorFlagged = false;

  if (!m_class && m_className.empty()) {
    func = ar->findFunction(m_name);
  } else {
    ClassScopePtr cls = ar->resolveClass(m_className);
    if (cls && cls->isVolatile()) {
      ar->getScope()->getVariables()
        ->setAttribute(VariableTable::NeedGlobalPointer);
    }
    if (!cls || cls->isRedeclaring()) {
      if (cls) {
        m_redeclaredClass = true;
      }
      if (!cls && ar->isFirstPass()) {
        ar->getCodeError()->record(self, CodeError::UnknownClass, self);
      }
      if (m_params) {
        m_params->inferAndCheck(ar, NEW_TYPE(Some), false);
        m_params->markParams(false);
      }
      return checkTypesImpl(ar, type, Type::Variant, coerce);
    }
    m_derivedFromRedeclaring = cls->derivesFromRedeclaring();
    m_validClass = true;

    if (m_name == "__construct") {
      // if the class is known, php will try to identify class-name ctor
      func = cls->findConstructor(ar, true);
    }
    else {
      func = cls->findFunction(ar, m_name, true, true);
    }

    if (func && !func->isStatic()) {
      ClassScopePtr clsThis = ar->getClassScope();
      FunctionScopePtr funcThis = ar->getFunctionScope();
      if (!clsThis ||
          (clsThis->getName() != m_className &&
           !clsThis->derivesFrom(ar, m_className, true, false)) ||
          funcThis->isStatic()) {
        func->setDynamic();
        if (ar->isFirstPass()) {
          ar->getCodeError()->record(self, CodeError::MissingObjectContext,
                                     self);
          errorFlagged = true;
        }
        func.reset();
      }
    }
  }
  if (!func || func->isRedeclaring()) {
    if (func) {
      m_redeclared = true;
      ar->getScope()->getVariables()->
        setAttribute(VariableTable::NeedGlobalPointer);
    }
    if (!func && !errorFlagged && ar->isFirstPass()) {
      ar->getCodeError()->record(self, CodeError::UnknownFunction, self);
    }
    if (m_params) {
      if (func) {
        FunctionScope::RefParamInfoPtr info =
          FunctionScope::GetRefParamInfo(m_name);
        ASSERT(info);
        for (int i = m_params->getCount(); i--; ) {
          if (info->isRefParam(i)) {
            m_params->markParam(i, canInvokeFewArgs());
          }
        }
      }
      m_params->inferAndCheck(ar, NEW_TYPE(Some), false);
    }
    return checkTypesImpl(ar, type, Type::Variant, coerce);
  }
  m_builtinFunction = (!func->isUserFunction() || func->isSepExtension());

  CHECK_HOOK(beforeSimpleFunctionCallCheck);

  m_valid = true;
  type = checkParamsAndReturn(ar, type, coerce, func);

  if (!m_valid && m_params) {
    m_params->markParams(false);
  }

  CHECK_HOOK(afterSimpleFunctionCallCheck);

  return type;
}

///////////////////////////////////////////////////////////////////////////////
// code generation functions

void SimpleFunctionCall::outputPHP(CodeGenerator &cg, AnalysisResultPtr ar) {
  outputLineMap(cg, ar);

  if (m_class || !m_className.empty()) {
    StaticClassName::outputPHP(cg, ar);
    cg_printf("::%s(", m_name.c_str());
  } else {

    if (cg.getOutput() == CodeGenerator::InlinedPHP ||
        cg.getOutput() == CodeGenerator::TrimmedPHP) {

      if (cg.getOutput() == CodeGenerator::TrimmedPHP &&
          cg.usingStream(CodeGenerator::PrimaryStream) &&
          Option::DynamicFunctionCalls.find(m_name) !=
          Option::DynamicFunctionCalls.end()) {
        int funcNamePos = Option::DynamicFunctionCalls[m_name];
        if (m_params && m_params->getCount() &&
            m_params->getCount() >= funcNamePos + 1) {
          if (funcNamePos == -1) funcNamePos = m_params->getCount() - 1;
          ExpressionPtr funcName = (*m_params)[funcNamePos];
          if (!funcName->is(Expression::KindOfScalarExpression)) {

            cg_printf("%s(", m_name.c_str());
            for (int i = 0; i < m_params->getCount(); i++) {
              if (i > 0) cg_printf(", ");
              if (i == funcNamePos) {
                cg_printf("%sdynamic_load(", Option::IdPrefix.c_str());
                funcName->outputPHP(cg, ar);
                cg_printf(")");
              } else {
                ExpressionPtr param = (*m_params)[i];
                if (param) param->outputPHP(cg, ar);
              }
            }
            cg_printf(")");
            return;
          }
        }
      }
      /* simptodo: I dunno
      if (m_type == RenderTemplateFunction && !m_template.empty()) {
        cg_printf("%s_%s(", m_name.c_str(),
                  Util::getIdentifier(m_template).c_str());
      } else if (m_type == RenderTemplateIncludeFunction) {
        string templateName = ar->getProgram()->getCurrentTemplate();
        cg_printf("%s_%s(", m_name.c_str(),
                  Util::getIdentifier(templateName).c_str());
      } else {
      */
        cg_printf("%s(", m_name.c_str());
        //}

    } else {
      cg_printf("%s(", m_name.c_str());
    }
  }

  if (m_params) m_params->outputPHP(cg, ar);
  cg_printf(")");
}

void SimpleFunctionCall::outputCPPParamOrderControlled(CodeGenerator &cg,
                                                       AnalysisResultPtr ar) {
  if (!m_class && m_className.empty()) {
    switch (m_type) {
    case ExtractFunction:
      cg_printf("extract(variables, ");
      FunctionScope::outputCPPArguments(m_params, cg, ar, 0, false);
      cg_printf(")");
      return;
    case CompactFunction:
      cg_printf("compact(variables, ");
      FunctionScope::outputCPPArguments(m_params, cg, ar, -1, true);
      cg_printf(")");
      return;
    default:
      break;
    }
  }
  bool volatileCheck = false;
  ClassScopePtr cls;
  if (!m_className.empty()) {
    cls = ar->findClass(m_className);
    if (cls && !ar->checkClassPresent(m_origClassName)) {
      volatileCheck = true;
      cls->outputVolatileCheckBegin(cg, ar, cls->getOriginalName());
    }
  }
  if (m_valid) {
    if (!m_className.empty()) {
      cg_printf("%s%s::", Option::ClassPrefix,
                cg.formatLabel(m_className).c_str());
      if (m_name == "__construct" && cls) {
        FunctionScopePtr func = cls->findConstructor(ar, true);
        cg_printf("%s%s(", Option::MethodPrefix, func->getId(cg).c_str());
      } else {
        cg_printf("%s%s(", Option::MethodPrefix,
                  cg.formatLabel(m_name).c_str());
      }
    } else {
      int paramCount = m_params ? m_params->getCount() : 0;
      if (m_name == "get_class" && ar->getClassScope() && paramCount == 0) {
        cg_printf("(\"%s\"", ar->getClassScope()->getOriginalName().c_str());
      } else if (m_name == "get_parent_class" && ar->getClassScope() &&
                 paramCount == 0) {
        const std::string parentClass = ar->getClassScope()->getParent();
        if (!parentClass.empty()) {
          cg_printf("(\"%s\"", ar->getClassScope()->getParent().c_str());
        } else {
          cg_printf("(false");
        }
      } else {
        if (m_noPrefix) {
          cg_printf("%s(", cg.formatLabel(m_name).c_str());
        }
        else {
          cg_printf("%s%s(",
                    m_builtinFunction ? Option::BuiltinFunctionPrefix :
                    Option::FunctionPrefix, cg.formatLabel(m_name).c_str());
        }
      }
    }
    FunctionScope::outputCPPArguments(m_params, cg, ar, m_extraArg,
                                      m_variableArgument, m_argArrayId);
    cg_printf(")");
  } else {
    if (!m_class && m_className.empty()) {
      if (m_redeclared && !m_dynamicInvoke) {
        if (canInvokeFewArgs()) {
          cg_printf("%s->%s%s_few_args(", cg.getGlobals(ar),
                    Option::InvokePrefix, cg.formatLabel(m_name).c_str());
          int left = Option::InvokeFewArgsCount;
          if (m_params && m_params->getCount()) {
            left -= m_params->getCount();
            cg_printf("%d, ", m_params->getCount());
            FunctionScope::outputCPPArguments(m_params, cg, ar, 0, false);
          } else {
            cg_printf("0");
          }
          for (int i = 0; i < left; i++) {
            cg_printf(", null_variant");
          }
          cg_printf(")");
          return;
        } else {
          cg_printf("%s->%s%s(", cg.getGlobals(ar), Option::InvokePrefix,
                    cg.formatLabel(m_name).c_str());
        }
      } else {
        cg_printf("invoke(\"%s\", ", m_name.c_str());
      }
    } else {
      bool inObj = m_parentClass && ar->getClassScope() &&
        !dynamic_pointer_cast<FunctionScope>(ar->getScope())->isStatic();
      if (m_redeclaredClass) {
        if (inObj) {  // parent is redeclared
          cg_printf("parent->%sinvoke(\"%s\",", Option::ObjectPrefix,
                    m_name.c_str());
        } else {
          cg_printf("%s->%s%s->%sinvoke(\"%s\", \"%s\",",
                    cg.getGlobals(ar),
                    Option::ClassStaticsObjectPrefix,
                    cg.formatLabel(m_className).c_str(),
                    Option::ObjectStaticPrefix,
                    m_className.c_str(),
                    m_name.c_str());
        }
      } else if (m_validClass) {
        if (inObj) {
          cg_printf("%s%s::%sinvoke(\"%s\",",
                    Option::ClassPrefix, cg.formatLabel(m_className).c_str(),
                    Option::ObjectPrefix, m_name.c_str());
        } else {
          cg_printf("%s%s::%sinvoke(\"%s\", \"%s\",",
                    Option::ClassPrefix, cg.formatLabel(m_className).c_str(),
                    Option::ObjectStaticPrefix,
                    m_className.c_str(),
                    m_name.c_str());
        }
      } else {
        if (m_class) {
          cg_printf("INVOKE_STATIC_METHOD(get_static_class_name(");
          if (m_class->is(KindOfScalarExpression)) {
            ASSERT(strcasecmp(dynamic_pointer_cast<ScalarExpression>(m_class)->
                              getString().c_str(), "static") == 0);
            cg_printf("\"static\"");
          } else {
            m_class->outputCPP(cg, ar);
          }
          cg_printf("), \"%s\",", m_name.c_str());
        } else {
          cg_printf("invoke_static_method(\"%s\", \"%s\",",
                    m_className.c_str(), m_name.c_str());
        }
      }
    }
    if ((!m_params) || (m_params->getCount() == 0)) {
      cg_printf("Array()");
    } else {
      FunctionScope::outputCPPArguments(m_params, cg, ar, -1, false);
    }
    bool needHash = true;
    if (!m_class && m_className.empty()) {
      needHash = !(m_redeclared && !m_dynamicInvoke);
    } else {
      needHash = m_validClass || m_redeclaredClass;
    }
    if (!needHash) {
      cg_printf(")");
    } else {
      cg.printf(", 0x%016llXLL)", hash_string_i(m_name.data(), m_name.size()));
    }
  }
  if (volatileCheck) {
    cls->outputVolatileCheckEnd(cg);
  }
}

void SimpleFunctionCall::outputCPPImpl(CodeGenerator &cg,
                                       AnalysisResultPtr ar) {
  bool linemap = outputLineMap(cg, ar, true);

  if (!m_lambda.empty()) {
    cg_printf("\"%s\"", m_lambda.c_str());
    if (linemap) cg_printf(")");
    return;
  }

  if (!m_class && m_className.empty()) {
    if (m_type == DefineFunction && m_params && m_params->getCount() >= 2) {
      ScalarExpressionPtr name =
        dynamic_pointer_cast<ScalarExpression>((*m_params)[0]);
      string varName;
      ExpressionPtr value = (*m_params)[1];
      if (name) {
        varName = name->getIdentifier();
        if (varName.empty()) {
          cg_printf("throw_fatal(\"bad define\")");
        } else if (m_dynamicConstant) {
          cg_printf("g->declareConstant(\"%s\", g->%s%s, ",
                    varName.c_str(), Option::ConstantPrefix,
                    varName.c_str());
          value->outputCPP(cg, ar);
          cg_printf(")");
        } else {
          bool needAssignment = true;
          bool isSystem = ar->getConstants()->isSystem(varName);
          if (isSystem ||
              ((!ar->isConstantRedeclared(varName)) && value->isScalar())) {
            needAssignment = false;
          }
          if (needAssignment) {
            cg_printf("%s%s = ", Option::ConstantPrefix, varName.c_str());
            value->outputCPP(cg, ar);
          }
        }
      } else {
        bool close = false;
        if (value->hasEffect()) {
          cg_printf("(id(");
          value->outputCPP(cg, ar);
          cg_printf("),");
          close = true;
        }
        cg_printf("throw_fatal(\"bad define\")");
        if (close) cg_printf(")");
      }
      if (linemap) cg_printf(")");
      return;
    }
    if (m_name == "func_num_args") {
      cg_printf("num_args");
      if (linemap) cg_printf(")");
      return;
    }

    switch (m_type) {
    case VariableArgumentFunction:
      {
        FunctionScopePtr func =
          dynamic_pointer_cast<FunctionScope>(ar->getScope());
        if (func) {
          cg_printf("%s(", m_name.c_str());
          func->outputCPPParamsCall(cg, ar, true);
          if (m_params) {
            cg_printf(",");
            m_params->outputCPP(cg, ar);
          }
          cg_printf(")");
          if (linemap) cg_printf(")");
          return;
        }
      }
      break;
    case FunctionExistsFunction:
    case ClassExistsFunction:
    case InterfaceExistsFunction:
      {
        bool literalString = false;
        string symbol;
        if (m_params && m_params->getCount() == 1) {
          ExpressionPtr value = (*m_params)[0];
          if (value->isScalar()) {
            ScalarExpressionPtr name =
              dynamic_pointer_cast<ScalarExpression>(value);
            if (name && name->isLiteralString()) {
              literalString = true;
              symbol = name->getLiteralString();
            }
          }
        }
        if (literalString) {
          switch (m_type) {
          case FunctionExistsFunction:
            {
              const std::string &lname = Util::toLower(symbol);
              bool dynInvoke = Option::DynamicInvokeFunctions.find(lname) !=
                Option::DynamicInvokeFunctions.end();
              if (!dynInvoke) {
                FunctionScopePtr func = ar->findFunction(lname);
                if (func) {
                  if (!func->isDynamic()) {
                    if (func->isRedeclaring()) {
                      const char *name = func->getName().c_str();
                      cg_printf("(%s->%s%s != invoke_failed_%s)",
                                cg.getGlobals(ar), Option::InvokePrefix,
                                name, name);
                      break;
                    }
                    cg_printf("true");
                    break;
                  }
                } else {
                  cg_printf("false");
                  break;
                }
              }
              cg_printf("f_function_exists(\"%s\")", lname.c_str());
            }
            break;
          case ClassExistsFunction:
            {
              // All the definite cases should have been resolved in
              // postOptimize().
              cg_printf("f_class_exists(\"%s\")",
                        Util::toLower(symbol).c_str());
            }
            break;
          case InterfaceExistsFunction:
            {
              // All the definite cases should have been resolved in
              // postOptimize().
              cg_printf("f_interface_exists(\"%s\")",
                        Util::toLower(symbol).c_str());
            }
            break;
          default:
            break;
          }
          if (linemap) cg_printf(")");
          return;
        }
      }
      break;
    case GetDefinedVarsFunction:
      cg_printf("get_defined_vars(variables)");
      if (linemap) cg_printf(")");
      return;
    default:
      break;
    }
  }

  outputCPPParamOrderControlled(cg, ar);
  if (linemap) cg_printf(")");
}

bool SimpleFunctionCall::canInvokeFewArgs() {
  // We can always change out minds about saying yes, but once we say
  // no, it sticks.
  if (m_dynamicInvoke ||
      (m_invokeFewArgsDecision && m_params &&
       m_params->getCount() > Option::InvokeFewArgsCount)) {
    m_invokeFewArgsDecision = false;
  }
  return m_invokeFewArgsDecision;
}
