#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

enum class TokenType {
    LiteralArit,
    LiteralBool,
    Keyword,
    Type,
    ArithmeticOp,
    AssignOp,
    ComparisonOp,
    Separator,
    Identifier,
};

enum class TokenSubType {
    Fn,
    Ret,
    If,
    Elif,
    Else,

    I32,
    Void,

    Plus,
    Minus,
    Multiply,
    Divide,

    Assign,
    AssignAdd,

    Equals,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,

    LCurlyBracket,
    RCurlyBracket,
    LParen,
    RParen,
    Comma,

    EndOfEnum,

    Identifier,
    I32Literal,
    True,
    False,
};

using TokenInfo = std::tuple<TokenType, TokenSubType>;
using TokenReturn = std::tuple<int, std::string_view>;

struct Token {
    Token(TokenType type, TokenSubType subType, std::string_view lexeme, int line);

    TokenType type;
    TokenSubType subType;
    std::string lexeme;
    int line;
};

class TokenIdentifier {
public:
    TokenIdentifier();

    TokenInfo identifyKeyword(std::string_view) const;
    TokenInfo identifyNumberLiteral(std::string_view) const;
    TokenInfo identifyArithmeticOperator(std::string_view) const;
    TokenInfo identifyAssignOperator(std::string_view) const;
    TokenInfo identifyComparisonOperator(std::string_view) const;

    TokenInfo identifySeparator(std::string_view) const;
private:
    std::unordered_map<std::string_view, TokenSubType> keywords;
    std::unordered_map<std::string_view, TokenSubType> types;
    std::unordered_map<std::string_view, TokenSubType> arithmeticOp;
    std::unordered_map<std::string_view, TokenSubType> assignOp;
    std::unordered_map<std::string_view, TokenSubType> comparisonOp;
    std::unordered_map<std::string_view, TokenSubType> separators;
};

class TokenParser {
public:
    TokenParser();

    std::vector<Token> consumeLine(const TokenIdentifier&, std::string_view);
private:

    TokenReturn parseWord(std::string_view t_input) const;
    TokenReturn parseNumber(std::string_view t_input) const;
    TokenReturn parseArithmeticOperator(std::string_view t_input) const;
    TokenReturn parseAssignOperator(std::string_view t_input) const;
    TokenReturn parseComparisonOperator(std::string_view t_input) const;
    TokenReturn parseSeparator(std::string_view t_input) const;

    int skipIndentation(std::string_view t_input) const;
    int rowIndex;
};

class Lexer {
public:
    Lexer();

    void tokenize(std::string_view source);
    void clear();
    const std::vector<Token>& getTokens() const;

private:
    std::vector<Token> tokens;
    TokenParser tokenParser;
    TokenIdentifier tokenIdentifier;
};