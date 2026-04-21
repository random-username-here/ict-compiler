/**
 * \file
 * \brief X-macro for generating IR op names
 */
#pragma once

// when adding new ops also see ir.cpp, there are some places where
// that op should be specified

#define ICT_FOR_ALL_OPS(X)\
    /* .-- id (must be > 0) */\
    /* |    .---- terminal?*/\
    /* |    |       .---- explicit type required?*/\
    /* |    |       |       .----------- name (OP_... are generated using this)*/\
    /* v    v       v       v               .------ Text representation name */\
    X(0x00, false,  false,  UNKNOWN_OP,     "?")\
    X(0x01, false,  false,  CONST,          "Const") /* type defaults to i64 */\
    X(0x02, true,   false,  RET,            "Ret")\
    X(0x03, false,  true,   ALLOCA,         "Alloca")\
    X(0x04, false,  true,   LOAD,           "Load")\
    X(0x05, false,  true,   STORE,          "Store")\
    X(0x06, false,  false,  CALL,           "Call")\
    X(0x07, false,  false,  ARGPTR,         "ArgPtr")\
    \
    X(0x10, false,  false,  ADD,            "Add")\
    X(0x11, false,  false,  SUB,            "Sub")\
    X(0x12, false,  false,  DIV,            "Div")\
    X(0x13, false,  false,  MUL,            "Mul")\
    X(0x14, false,  false,  MOD,            "Mod")\
    \
    X(0x20, false,  false,  LT,             "Lt")\
    X(0x21, false,  false,  GT,             "Gt")\
    X(0x22, false,  false,  GE,             "Ge")\
    X(0x23, false,  false,  LE,             "Le")\
    X(0x24, false,  false,  EQ,             "Eq")\
    X(0x25, false,  false,  NEQ,            "Neq")\
    \
    X(0x30, true,   false,  BR,             "Br")\
    X(0x31, true,   false,  BC,             "Bc")\
    \
    X(0x40, false,  false,  DEBUG_PRINT,    "DebugPrint")\

