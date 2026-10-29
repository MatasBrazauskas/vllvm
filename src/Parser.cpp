#include "Parser.hpp"
#include "Lexer.hpp"
#include "Utils.hpp"

Parser::Parser(const std::vector<Token>& t_tokens): c{t_tokens} {}

bool Consumer::checkType(const TokenType t_type) const {
    return tokens_[index_].type == t_type;
}

bool Consumer::checkSubType(const TokenSubType t_subType) const {
    return tokens_[index_].subType == t_subType;
}

bool Consumer::matchType(const TokenType t_subType) {
    if (checkType(t_subType)) {
        index_++;
        return true;
    }
    return false;
}

bool Consumer::matchSubType(const TokenSubType t_subType) {
    if (checkSubType(t_subType)) {
        index_++;
        return true;
    }
    return false;
}

const Token& Consumer::consumeType(const TokenType t_type) {
    if (checkType(t_type)) {
        return tokens_[index_++];
    }

    throw std::runtime_error("Error with consume function");
}

const Token& Consumer::consumeSubType(const TokenSubType t_subType) {
    if (checkSubType(t_subType)) {
        return tokens_[index_++];
    }

    throw std::runtime_error("Error with consume function");
}
bool Consumer::isEnd() const {
    return index_ == tokens_.size();
}

Program Parser::parse() {
    Program program;

    while (not c.isEnd()) {
        program.functions.push_back(parseFunction());
    }

    return program;
}

std::unique_ptr<Expr> Parser::parseExpr() {
    if (c.checkType(TokenType::LiteralArit)) {
        const auto literalToken = c.consumeType(TokenType::LiteralArit);
        const auto literalType = getType(literalToken.subType);

        return std::make_unique<LiteralExpr>(literalType, literalToken.lexeme);
    }

    if (c.checkSubType(TokenSubType::Identifier)) {
        const auto identifierToken = c.consumeSubType(TokenSubType::Identifier);

        if (c.checkSubType(TokenSubType::LParen)) {
            auto callExpr = parseCallExpr(identifierToken.lexeme);
            return std::move(callExpr);
        } else {
            return std::make_unique<IdentExpr>(identifierToken.lexeme);
        }
    }

    throw std::runtime_error("Unknown expression type");
}

std::unique_ptr<CallExpr> Parser::parseCallExpr(const std::string& t_funcName) {
    auto callExpr = std::make_unique<CallExpr>(t_funcName);

    c.consumeSubType(TokenSubType::LParen);

    while (not c.checkSubType(TokenSubType::RParen)) {
        auto expr = parseExpr();
        callExpr->addArgument(std::move(expr));

        if (c.checkSubType(TokenSubType::Comma)) {
            c.matchSubType(TokenSubType::Comma);
        }
    }

    c.consumeSubType(TokenSubType::RParen);

    return callExpr;
}

std::unique_ptr<BoolExpr> Parser::parseBoolExpression() {
    if (c.checkType(TokenType::LiteralBool)) {
        const auto boolLiteralToken = c.consumeType(TokenType::LiteralBool);
        const auto flag = boolLiteralToken.subType == TokenSubType::True;

        return std::make_unique<BoolLiteral>(flag);
    }

    if (c.checkSubType(TokenSubType::Identifier) or c.checkType(TokenType::LiteralArit)) {
        auto lf = parseExpr();

        const auto opToken = c.consumeType(TokenType::ComparisonOp);
        const auto op = getBoolOperator(opToken.subType);

        auto rh = parseExpr();

        return std::make_unique<ComparingBoolExpr>(std::move(lf), op, std::move(rh));
    }

    throw std::runtime_error("Unsupported boolean expression type.");
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (c.checkType(TokenType::Type)) {
        auto varDecl = parseVar();
        return std::move(varDecl);
    }

    if (c.matchSubType(TokenSubType::Ret)) {
        return std::make_unique<ReturnStmt>(parseExpr());
    }

    if (c.checkSubType(TokenSubType::If)) {
        return parseCondition();
    }

    if (c.checkSubType(TokenSubType::Identifier)) {
        return parseVarAssign();
    }

    throw std::runtime_error("Unsupported statement parsing");
}

std::unique_ptr<VarTypeStmt> Parser::parseVarTypeStmt() {
    const auto typeNameToken = c.consumeType(TokenType::Type);
    const auto typeName = getType(typeNameToken.subType);

    return std::make_unique<VarTypeStmt>(typeName);
}

std::unique_ptr<VarDeclStmt> Parser::parseVarDecl() {
    std::unique_ptr<VarTypeStmt> varTypeDecl = parseVarTypeStmt();
    const auto identifierName = c.consumeSubType(TokenSubType::Identifier).lexeme;

    return std::make_unique<VarDeclStmt>(std::move(varTypeDecl), identifierName);
}

std::unique_ptr<VarInitStmt> Parser::parseVar() {
    auto varDecl = parseVarDecl();

    c.consumeSubType(TokenSubType::LCurlyBracket);

    std::unique_ptr<Expr> expr ;

    if (not c.checkSubType(TokenSubType::RCurlyBracket)) {
        expr = parseExpr();
    }

    c.consumeSubType(TokenSubType::RCurlyBracket);

    return std::make_unique<VarInitStmt>(std::move(varDecl), std::move(expr));
}

std::unique_ptr<VarAssignStmt> Parser::parseVarAssign() {
    const auto identifierToken = c.consumeSubType(TokenSubType::Identifier);
    const auto assignToken = c.consumeType(TokenType::AssignOp);
    const auto assignOp = getAssignOperator(assignToken.subType);

    std::unique_ptr<Expr> expr = parseExpr();
    return std::make_unique<VarAssignStmt>(identifierToken.lexeme, assignOp, std::move(expr));
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    c.consumeSubType(TokenSubType::LCurlyBracket);
    auto blockStmt = std::make_unique<BlockStmt>();

    while (not c.matchSubType(TokenSubType::RCurlyBracket)) {
        auto stmt = parseStatement();
        blockStmt->addStmt(std::move(stmt));
    }

    return std::move(blockStmt);
}

FnDeclStmt Parser::parseFunction() {
    c.consumeSubType(TokenSubType::Fn);

    const auto fnName = c.consumeSubType(TokenSubType::Identifier).lexeme;

    c.consumeSubType(TokenSubType::LParen);

    std::vector<std::unique_ptr<VarDeclStmt>> parameters;

    while (not c.checkSubType(TokenSubType::RParen)) {
        auto varDecl = parseVarDecl();
        parameters.emplace_back(std::move(varDecl));

        c.matchSubType(TokenSubType::Comma);
    }

    c.consumeSubType(TokenSubType::RParen);

    auto returnType = parseVarTypeStmt();

    std::unique_ptr<BlockStmt> body = parseBlock();

    return FnDeclStmt{fnName, std::move(parameters), std::move(returnType), std::move(body)};
}

std::unique_ptr<ConditionStmt> Parser::parseIfStmt() {
    if (c.matchSubType(TokenSubType::If) == false) {
        c.consumeSubType(TokenSubType::Elif);
    }

    std::unique_ptr<BoolExpr> condition = parseBoolExpression();
    std::unique_ptr<BlockStmt> block = parseBlock();

    return std::make_unique<ConditionStmt>(std::move(condition), std::move(block));
}

std::unique_ptr<IfStmt> Parser::parseCondition() {

    auto ifStmt = parseIfStmt();
    auto condition = std::make_unique<IfStmt>(std::move(ifStmt));

    while (c.checkSubType(TokenSubType::Elif)) {
        auto elifStmt = parseIfStmt();
        condition->addElifStmt(std::move(elifStmt));
    }

    std::unique_ptr<BlockStmt> block;

    if (c.matchSubType(TokenSubType::Else)) {
        block = parseBlock();
    }

    condition->addElseBlock(std::move(block));

    return condition;
}
