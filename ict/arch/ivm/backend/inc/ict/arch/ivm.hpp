/**
 * \file
 * \brief Operator types for IVM
 */
#pragma once
namespace ict::ar::ivm {

#define ICT_IVM_FOR_ALL_OPS(X)\
    /*      id  inv     from    name            textname            instr */\
    X(      -1, false,  0,      IVM_TOSTACK,    "IVM.ToStack",      nullptr)\
    X(      -2, false,  0,      IVM_FROMSTACK,  "IVM.FromStack",    nullptr)\
    X(      -3, false,  0,      IVM_DEBUG_PRINT_INT, "IVM.DebugPrintInt",  "call __ict.debugPrintInt")\
    X(      -4, false,  0,      IVM_DEBUG_PRINT_CHAR, "IVM.DebugPrintChar",  "call __ict.debugPrintChar")\
    \
    X(  -0x121, false,  0,      IVM_R_PUSH,     "IVM.R.Push",        "push")\
    \
    X(  -0x101, false,  ADD,    IVM_R_ADD,      "IVM.R.Add",        "add")\
    X(  -0x102, false,  SUB,    IVM_R_SUB,      "IVM.R.Sub",        "sub")\
    X(  -0x103, false,  MUL,    IVM_R_MUL,      "IVM.R.Mul",        "mul")\
    X(  -0x103, false,  DIV,    IVM_R_DIV,      "IVM.R.Div",        "div")\
    X(  -0x104, false,  MOD,    IVM_R_MOD,      "IVM.R.Mod",        "mod")\
    \
    X(  -0x110, false,  EQ,     IVM_R_EQ,       "IVM.R.Eq",         "eq")\
    X(  -0x111, false,  0,      IVM_R_ISZERO,   "IVM.R.IsZero",     "iszero")\
    X(  -0x112, true,   LT,     IVM_R_LT,       "IVM.R.Lt",         "lt")\
    X(  -0x113, true,   LE,     IVM_R_LE,       "IVM.R.Le",         "le")\
    X(  -0x114, true,   GT,     IVM_R_GT,       "IVM.R.Gt",         "gt")\
    X(  -0x115, true,   GE,     IVM_R_GE,       "IVM.R.Ge",         "ge")\
    \
    X(  -0x130, false,  0,      IVM_R_RET,      "IVM.R.Ret",        "ret")\
    X(  -0x137, false,  0,      IVM_R_RJMP,     "IVM.R.RJmp",       "rjmp")\
    X(  -0x138, false,  0,      IVM_R_RJNZ,     "IVM.R.RJnz",       "rjnz")\
    \
    X(  -0x147, true,   STORE,  IVM_R_PUT64,    "IVM.R.Put64",      "put64")\
    X(  -0x143, false,  LOAD,   IVM_R_GET64,    "IVM.R.Get64",      "get64")\
    \
    X(  -0x160, false,  0,      IVM_R_SADD,     "IVM.R.SAdd",        "sadd")\
    X(  -0x15b, false,  0,      IVM_R_SFLOAD64, "IVM.R.SFLoad64",    "sfload64")\
    X(  -0x15f, false,  0,      IVM_R_SFSTORE64, "IVM.R.SFStore64",    "sfstore64")\
    \
    X(  -0x154, false,  0,      IVM_R_SPUSH,    "IVM.R.SPush",      "spush")\
    X(  -0x155, false,  0,      IVM_R_SPOP,     "IVM.R.SPop",       "spop")\
    X(  -0x152, false,  0,      IVM_R_GSP,      "IVM.R.GSP",        "gsp")\
    X(  -0x156, false,  0,      IVM_R_SFBEGIN,  "IVM.R.SFBegin",    "sfbegin")\
    X(  -0x157, false,  0,      IVM_R_SFEND,    "IVM.R.SFEnd",      "sfend")\

#define X(id, inv, from, name, textname, instr) constexpr int name = id;
ICT_IVM_FOR_ALL_OPS(X)
#undef X

};
