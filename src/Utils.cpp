#include "Utils.hpp"

static const std::unordered_map<TokenSubType, Types> types = {
    {TokenSubType::I32, Types::I32},
    {TokenSubType::I32Literal, Types::I32},
    {TokenSubType::True, Types::Bool},
    {TokenSubType::False, Types::Bool},
    {TokenSubType::Void, Types::Void},
};

static const std::unordered_map<TokenSubType, BoolOperator> boolOp = {
    {TokenSubType::Less, BoolOperator::Less},
    {TokenSubType::LessEqual, BoolOperator::LessEq},
    {TokenSubType::Greater, BoolOperator::Greater},
    {TokenSubType::GreaterEqual, BoolOperator::GreaterEq},
    {TokenSubType::Equals, BoolOperator::Equal},
};

static const std::unordered_map<TokenSubType, ArithmeticOperator> arithOp = {
    {TokenSubType::Plus, ArithmeticOperator::Plus},
    {TokenSubType::Minus, ArithmeticOperator::Minus},
    {TokenSubType::Divide, ArithmeticOperator::Multiply},
};

static const std::unordered_map<TokenSubType, AssignOperator> assignOp = {
    {TokenSubType::Assign, AssignOperator::Assign},
    {TokenSubType::AssignAdd, AssignOperator::AssignAdd},
};

Types getType(const TokenSubType t_type) {
    const auto it = types.find(t_type);
    return it->second;
}

BoolOperator getBoolOperator(const TokenSubType t_boolOp) {
    const auto it = boolOp.find(t_boolOp);
    return it->second;
}

ArithmeticOperator getArithmeticOperator(const TokenSubType t_arit) {
    const auto it = arithOp.find(t_arit);
    return it->second;
}

AssignOperator getAssignOperator(const TokenSubType t_ass) {
    const auto it = assignOp.find(t_ass);
    return it->second;
}