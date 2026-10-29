#pragma once

#include "Ast.hpp"

#include <memory>
#include <vector>

enum class TokenType;
enum class TokenSubType;
struct Token;


class Consumer {
public:
    explicit Consumer(const std::vector<Token>& t_tokens): tokens_{t_tokens}, index_{0} {}
    bool checkType(TokenType) const;
    bool checkSubType(TokenSubType) const;

    bool matchType(TokenType);
    bool matchSubType(TokenSubType);

    const Token& consumeType(TokenType);
    const Token& consumeSubType(TokenSubType);

    bool isEnd() const;
private:
    const std::vector<Token>& tokens_;
    std::size_t index_;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>&);
    Program parse();
private:
    Consumer c;

    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<CallExpr> parseCallExpr(const std::string&);

    std::unique_ptr<BoolExpr> parseBoolExpression();

    std::unique_ptr<Stmt> parseStatement();

    std::unique_ptr<VarTypeStmt> parseVarTypeStmt();
    std::unique_ptr<VarDeclStmt> parseVarDecl();
    std::unique_ptr<VarInitStmt> parseVar();
    std::unique_ptr<VarAssignStmt> parseVarAssign();

    std::unique_ptr<BlockStmt> parseBlock();
    FnDeclStmt parseFunction();

    std::unique_ptr<IfStmt> parseCondition();
    std::unique_ptr<ConditionStmt> parseIfStmt();
};