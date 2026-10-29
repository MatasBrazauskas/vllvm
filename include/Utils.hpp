#pragma once

#include <Lexer.hpp>

enum class Types {
    I32,
    Bool,
    Void,
};

enum class BoolOperator {
    Less,
    Greater,
    LessEq,
    GreaterEq,
    Equal,
};

enum class ArithmeticOperator {
    Plus,
    Minus,
    Multiply,
    Divide,

    Assign,
    AssignAdd,
};

enum class AssignOperator {
    Assign,
    AssignAdd,
};

//extern const std::unordered_map<TokenSubType, Types> types;

Types getType(TokenSubType);
BoolOperator getBoolOperator(TokenSubType);
ArithmeticOperator getArithmeticOperator(TokenSubType);
AssignOperator getAssignOperator(TokenSubType);