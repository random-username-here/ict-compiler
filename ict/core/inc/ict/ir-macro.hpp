/**
 * \file
 * \brief X-macro for generating IR op names
 */
#pragma once

#define ICT_FOR_ALL_OPS(X)\
    /*  id  term?   vreg?   name            textname */\
    X(0x00, false,  true,   UNKNOWN_OP,     "?")\
    X(0x01, false,  true,   CONST,          "Const")\
    X(0x02, false,  false,  RET,            "Ret")\
    X(0x03, false,  true,   ALLOCA,         "Alloca")\
    X(0x04, false,  true,   LOAD,           "Load")\
    X(0x05, false,  false,  STORE,          "Store")\
    \
    X(0x10, false,  true,   ADD,            "Add")\
    X(0x11, false,  true,   SUB,            "Sub")\
    X(0x12, false,  true,   DIV,            "Div")\
    X(0x13, false,  true,   MUL,            "Mul")\
    X(0x14, false,  true,   MOD,            "Mod")\
    \
    X(0x20, false,  true,   LT,             "Lt")\
    X(0x21, false,  true,   GT,             "Gt")\
    X(0x22, false,  true,   GE,             "Ge")\
    X(0x23, false,  true,   LE,             "Le")\
    X(0x24, false,  true,   EQ,             "Eq")\
    X(0x25, false,  true,   NEQ,            "Neq")\
    \
    X(0x30, true,   false,  BR,             "Br")\
    X(0x31, true,   false,  BC,             "Bc")\
    \
    X(0x40, false,  false,  DEBUG_PRINT,    "DebugPrint")\

