#include "Lexer.hpp"

#include <iostream>
#include <stdexcept>
#include <cassert>
#include <flat_map>

using namespace std::string_literals;

namespace {
    bool isLetter(const char t_inputChar) {
        if (t_inputChar >= 'a' && t_inputChar <= 'z') {
            return true;
        }

        if (t_inputChar >= 'A' && t_inputChar <= 'Z') {
            return true;
        }
        return false;
    }

    bool isDigit(const char t_inputChar) {
        if (t_inputChar >= '0' && t_inputChar <= '9') {
            return true;
        }

        return false;
    }

    bool isWord(const char t_inputChar) {
        if (isLetter(t_inputChar)) {
            return true;
        }

        if (isDigit(t_inputChar)) {
            return true;
        }

        if (t_inputChar == '_') {
            return true;
        }

        return false;
    }

    bool isArithmeticOperator(const char t_inputChar) {
        if (t_inputChar == '+' or t_inputChar == '-') {
            return true;
        }

        if (t_inputChar == '*' or t_inputChar == '/') {
            return true;
        }

        return false;
    }

    bool isAssigmentOperator(const char t_inputChar, const char t_s) {
        if (t_inputChar == '=') {
            return true;
        }

        if (isArithmeticOperator(t_inputChar) and t_s == '=') {
            return true;
        }

        return false;
    }

    bool isLogicalOperator(const char t_inputChar, const char t_s) {
        if (t_inputChar == '=' and t_s == '=') {
            return true;
        }

        if (t_inputChar == '>' or t_inputChar == '<') {
            return true;
        }

        return false;
    }

    bool isSeparator(const char t_inputChar) {
        if (t_inputChar == ',' or t_inputChar == ';') {
            return true;
        }

        if (t_inputChar == '(' or t_inputChar == ')') {
            return true;
        }

        if (t_inputChar == '{' or t_inputChar == '}') {
            return true;
        }

        return false;
    }

    bool isWhiteSpace(const char t_inputChar) {
        return t_inputChar == ' ' || t_inputChar == '\r' || t_inputChar == '\t';
    }
}

Token::Token(const TokenType t_type, const TokenSubType t_subType, const std::string_view t_lexeme, const int t_line)
    : type{t_type}, subType{t_subType}, lexeme{t_lexeme}, line{t_line} {}

TokenIdentifier::TokenIdentifier() {
    keywords = {
        {"fn", TokenSubType::Fn},
        {"ret", TokenSubType::Ret},
        {"if", TokenSubType::If},
        {"elif", TokenSubType::Elif},
        {"else", TokenSubType::Else},
    };

    types = {
        {"i32", TokenSubType::I32},
        {"void", TokenSubType::Void},
    };

    arithmeticOp = {
        {"+", TokenSubType::Plus},
        {"-", TokenSubType::Minus},
        {"*", TokenSubType::Multiply},
        {"/", TokenSubType::Divide},
    };

    assignOp = {
        {"=", TokenSubType::Assign},
        {"+=", TokenSubType::AssignAdd},
    };

    comparisonOp = {
        {"==", TokenSubType::Equals},
        {"<", TokenSubType::Less},
        {">", TokenSubType::Greater},
        {">=", TokenSubType::LessEqual},
        {"<=", TokenSubType::GreaterEqual},
    };

    separators = {
        {"{", TokenSubType::LCurlyBracket},
        {"}", TokenSubType::RCurlyBracket},
        {"(", TokenSubType::LParen},
        {")", TokenSubType::RParen},
        {",", TokenSubType::Comma},
    };

    if (keywords.size() + types.size() + arithmeticOp.size() + assignOp.size() + comparisonOp.size() + separators.size() != std::to_underlying(TokenSubType::EndOfEnum)) {
        throw std::runtime_error{"Mismatch between the enum and maps."};
    }
}

std::tuple<TokenType, TokenSubType> TokenIdentifier::identifyKeyword(const std::string_view t_keyWord) const {
    if (const auto it = keywords.find(t_keyWord); it != keywords.end()) {
        return std::make_tuple(TokenType::Keyword, it->second);
    }

    if (const auto it = types.find(t_keyWord); it != types.end()) {
        return std::make_tuple(TokenType::Type, it->second);
    }

    if (t_keyWord == "false") {
        return std::make_tuple(TokenType::LiteralBool, TokenSubType::False);
    } else if (t_keyWord == "true") {
        return std::make_tuple(TokenType::LiteralBool, TokenSubType::True);
    }

    return std::make_tuple(TokenType::Identifier, TokenSubType::Identifier);
}

TokenInfo TokenIdentifier::identifyNumberLiteral(const std::string_view t_word) const {
    return std::make_tuple(TokenType::LiteralArit, TokenSubType::I32Literal);
}

TokenInfo TokenIdentifier::identifyAssignOperator(const std::string_view t_word) const {
    const auto it = assignOp.find(t_word);
    return std::make_tuple(TokenType::AssignOp, it->second);
}

TokenInfo TokenIdentifier::identifyArithmeticOperator(const std::string_view t_word) const {
    const auto it = arithmeticOp.find(t_word);
    return std::make_tuple(TokenType::ArithmeticOp, it->second);
}

TokenInfo TokenIdentifier::identifyComparisonOperator(const std::string_view t_word) const {
    const auto it = comparisonOp.find(t_word);
    return std::make_tuple(TokenType::ComparisonOp, it->second);
}

TokenInfo TokenIdentifier::identifySeparator(const std::string_view t_word) const {
    const auto it = separators.find(t_word);
    return std::make_tuple(TokenType::Separator, it->second);
}

TokenParser::TokenParser(): rowIndex{0} {}

std::vector<Token> TokenParser::consumeLine(const TokenIdentifier& t_tokenIdent, std::string_view t_line) {
    std::vector<Token> result;

    for (int index = 0; index < t_line.length();) {
        if (isLetter(t_line[index])) {
            const auto [newIndex, token] = parseWord(t_line.substr(index));
            const auto [type, subType] = t_tokenIdent.identifyKeyword(token);

            index += newIndex;

            result.emplace_back(type, subType, token, rowIndex);
        } else if (isDigit(t_line[index])) {
            const auto [newIndex, token]= parseNumber(t_line.substr(index));
            const auto [type, subType] = t_tokenIdent.identifyNumberLiteral(token);

            index += newIndex;

            result.emplace_back(type, subType, token, rowIndex);
        } else if (isLogicalOperator(t_line[index], index + 1 >= t_line.size() ? '\0' : t_line[index+1])) {
            const auto [newIndex, token] = parseComparisonOperator(t_line.substr(index));
            const auto [type, subType] = t_tokenIdent.identifyComparisonOperator(token);

            index += newIndex;

            result.emplace_back(type, subType, token, rowIndex);
        } else if (isAssigmentOperator(t_line[index], index + 1 >= t_line.size() ? '\0' :t_line[index+1])) {
            const auto [newIndex, token] = parseAssignOperator(t_line.substr(index));
            const auto [type, subType] = t_tokenIdent.identifyAssignOperator(token);

            index += newIndex;

            result.emplace_back(type, subType, token, rowIndex);
        } else if (isArithmeticOperator(t_line[index])) {
            const auto [newIndex, token] = parseArithmeticOperator(t_line.substr(index));
            const auto [type, subType] = t_tokenIdent.identifyArithmeticOperator(token);

            index += newIndex;

            result.emplace_back(type, subType, token, rowIndex);
        } else if (isSeparator(t_line[index])) {
            const auto [newIndex, token] = parseSeparator(t_line.substr(index));
            const auto [type, subType] = t_tokenIdent.identifySeparator(token);

            index += newIndex;

            result.emplace_back(type, subType, token, rowIndex);
        } else if (isWhiteSpace(t_line[index])) {
            index += skipIndentation(t_line.substr(index));
        } else {
            throw std::runtime_error("Lexer::tokenize() error");
        }
    }

    rowIndex++;

    return result;
}


TokenReturn TokenParser::parseWord(const std::string_view t_input) const {
    int index{};

    while (index < static_cast<int>(t_input.length()) and isWord(t_input[index])) {
        ++index;
    }

    const auto token = t_input.substr(0, index);
    return std::make_tuple(index, token);
}

TokenReturn TokenParser::parseNumber(const std::string_view t_input) const {
    int index{};

    while (index < static_cast<int>(t_input.length()) and isDigit(t_input[index])) {
        ++index;
    }

    const auto token = t_input.substr(0, index);
    return std::make_tuple(index, token);
}

TokenReturn TokenParser::parseArithmeticOperator(const std::string_view t_input) const {
    int index{1};

    if (index + 1 < static_cast<int>(t_input.length()) and t_input[index] == '=') {
        index++;
    }

    const auto subStr = t_input.substr(0, index);
    return std::make_tuple(index, subStr);
}

TokenReturn TokenParser::parseAssignOperator(const std::string_view t_input) const {
    int index{1};
    if (t_input[0] != '=') {
        index++;
    }

    const auto subStr = t_input.substr(0, index);
    return std::make_tuple(index, subStr);
}

TokenReturn TokenParser::parseComparisonOperator(std::string_view t_input) const {
    int index{1};

    if (t_input[0] == '=') {
        index++;
    }

    if (t_input[index] == '=') {
        index++;
    }

    const auto subStr= t_input.substr(0, index);
    return std::make_tuple(index, subStr);
}

TokenReturn TokenParser::parseSeparator(const std::string_view t_input) const {
    if (t_input[0] == '.' and t_input.length() > 1 and t_input[1] == '.') {
        const auto token = t_input.substr(0, 2);
        return std::make_tuple(2, token);
    }
    const auto token = t_input.substr(0, 1);
    return std::make_tuple(1, token);
}

int TokenParser::skipIndentation(const std::string_view t_input) const {
    const auto it = std::ranges::find_if(t_input, isWhiteSpace);
    if (it == t_input.end()) {
        return t_input.length();
    }

    return std::distance(t_input.begin(), it) + 1;
}


Lexer::Lexer() {}

void Lexer::tokenize(const std::string_view source) {
    const auto newTokens = tokenParser.consumeLine(tokenIdentifier, source);

    tokens.insert(tokens.end(), newTokens.begin(),newTokens.end());
}

void Lexer::clear() {
    tokens.clear();
}

const std::vector<Token>& Lexer::getTokens() const {
    return tokens;
}
