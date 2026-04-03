/**
 * \file
 * \brief Operator types for IVM
 */
#pragma once
namespace ict::ar::ivm {

#define ICT_IVM_FOR_ALL_OPS(X)\
    /*      id  from    name            textname            instr */\
    X(      -1, 0,      IVM_TOSTACK,    "IVM.ToStack",      nullptr)\
    X(      -2, 0,      IVM_FROMSTACK,  "IVM.FromStack",    nullptr)\
    X(      -3, DEBUG_PRINT, IVM_DEBUG_PRINT, "IVM.DebugPrint",   nullptr)\
    \
    X(  -0x121, 0,      IVM_R_PUSH,     "IVM.R.Push",        "push")\
    \
    X(  -0x101, ADD,    IVM_R_ADD,      "IVM.R.Add",        "add")\
    X(  -0x102, SUB,    IVM_R_SUB,      "IVM.R.Sub",        "sub")\
    X(  -0x103, MUL,    IVM_R_MUL,      "IVM.R.Mul",        "mul")\
    X(  -0x103, DIV,    IVM_R_DIV,      "IVM.R.Div",        "div")\
    X(  -0x104, MOD,    IVM_R_MOD,      "IVM.R.Mod",        "mod")\
    \
    X(  -0x110, EQ,     IVM_R_EQ,       "IVM.R.Eq",         "eq")\
    X(  -0x111, 0,      IVM_R_ISZERO,   "IVM.R.IsZero",     "iszero")\
    X(  -0x112, LT,     IVM_R_LT,       "IVM.R.Lt",         "lt")\
    X(  -0x113, LE,     IVM_R_LE,       "IVM.R.Le",         "le")\
    X(  -0x114, GT,     IVM_R_GT,       "IVM.R.Gt",         "gt")\
    X(  -0x115, GE,     IVM_R_GE,       "IVM.R.Ge",         "ge")\
    \
    X(  -0x130, 0,      IVM_R_RET,      "IVM.R.Ret",        "ret")\
    X(  -0x137, 0,      IVM_R_RJMP,     "IVM.R.RJmp",       "rjmp")\
    X(  -0x138, 0,      IVM_R_RJNZ,     "IVM.R.RJnz",       "rjnz")\
    \
    X(  -0x147, STORE,  IVM_R_PUT64,    "IVM.R.Put64",      "put64")\
    X(  -0x143, LOAD,   IVM_R_GET64,    "IVM.R.Get64",      "get64")\
    \
    X(  -0x160, 0,      IVM_R_SADD,     "IVM.R.SAdd",        "sadd")\
    X(  -0x15b, 0,      IVM_R_SFLOAD64, "IVM.R.SFLoad64",    "sfload64")\
    X(  -0x15f, 0,      IVM_R_SFSTORE64, "IVM.R.SFStore64",    "sfstore64")\

#define X(id, from, name, textname, instr) constexpr int name = id;
ICT_IVM_FOR_ALL_OPS(X)
#undef X

};
