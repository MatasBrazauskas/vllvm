#pragma once

#include "Lexer.hpp"
#include "Utils.hpp"

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

class Expr {
public:
    virtual ~Expr() noexcept = default;
};

class LiteralExpr : public Expr {
public:
    explicit LiteralExpr(Types t_literalTypes, const std::string& t_literalValue)
        : literalType{t_literalTypes}, literalValue{t_literalValue} {}

    Types literalType;
    std::string literalValue;
};

class IdentExpr : public Expr {
public:
    explicit IdentExpr(const std::string& t_identName): identName{t_identName} {}

    std::string identName;
};

class CallExpr: public Expr {
public:
    explicit CallExpr(const std::string& t_funcName): functionName{t_funcName} {}

    void addArgument(std::unique_ptr<Expr> t_argument) {
        args.emplace_back(std::move(t_argument));
    }

    std::string functionName;
    std::vector<std::unique_ptr<Expr>> args;
};

class BoolExpr {
public:
    virtual ~BoolExpr() noexcept = default;
};

class BoolLiteral : public BoolExpr {
public:
    explicit BoolLiteral(const bool t_isTrue): isTrue{t_isTrue} {}
    bool isTrue;
};

class ComparingBoolExpr : public BoolExpr {
public:
    explicit ComparingBoolExpr(std::unique_ptr<Expr> t_lh, const BoolOperator t_op, std::unique_ptr<Expr> t_rh)
        : lh{std::move(t_lh)}, boolOp{t_op}, rh{std::move(t_rh)} {}

    std::unique_ptr<Expr> lh;
    BoolOperator boolOp;
    std::unique_ptr<Expr> rh;
};

class Stmt {
public:
    virtual ~Stmt() noexcept = default;
};

class VarTypeStmt : public Stmt {
public:
    explicit VarTypeStmt(Types t_type): type{t_type} {}

    Types type;
};

class VarDeclStmt : public Stmt {
public:
    explicit VarDeclStmt(std::unique_ptr<VarTypeStmt> t_i, const std::string& t_identName)
        : varTypeStmt{std::move(t_i)}, identifierName{t_identName} {}

    std::unique_ptr<VarTypeStmt> varTypeStmt;
    std::string identifierName;
};

class VarInitStmt : public Stmt {
public:
    explicit VarInitStmt(std::unique_ptr<VarDeclStmt> t_varDecl, std::unique_ptr<Expr> t_initializer)
        : varDecl{std::move(t_varDecl)}, initializer{std::move(t_initializer)} {}


    std::unique_ptr<VarDeclStmt> varDecl;
    std::unique_ptr<Expr> initializer;
};

class VarAssignStmt : public Stmt {
public:
    explicit VarAssignStmt(const std::string& t_name, const AssignOperator t_op, std::unique_ptr<Expr> t_assignment)
        : name{t_name}, assOp{t_op}, assignment{std::move(t_assignment)} {}

    std::string name;
    AssignOperator assOp;
    std::unique_ptr<Expr> assignment;
};

class ReturnStmt : public Stmt {
public:
    explicit ReturnStmt(std::unique_ptr<Expr> t_returnExpr): returnExpr{std::move(t_returnExpr)} {}

    std::unique_ptr<Expr> returnExpr;
};

class BlockStmt : public Stmt {
public:
    BlockStmt() = default;

    void addStmt(std::unique_ptr<Stmt> t_stmt) {
        statements.push_back(std::move(t_stmt));
    }

    std::vector<std::unique_ptr<Stmt>> statements;
};

class FnDeclStmt : public Stmt {
public:
    explicit FnDeclStmt(const std::string& t_fnName, std::vector<std::unique_ptr<VarDeclStmt>> t_parameters, std::unique_ptr<VarTypeStmt> t_returnType, std::unique_ptr<BlockStmt> t_body)
        : functionName{t_fnName}, parameters{std::move(t_parameters)}, returnType{std::move(t_returnType)}, body{std::move(t_body)} {}

    std::string functionName;
    std::vector<std::unique_ptr<VarDeclStmt>> parameters;
    std::unique_ptr<VarTypeStmt> returnType;
    std::unique_ptr<BlockStmt> body;
};

class ConditionStmt : public Stmt {
public:
    explicit ConditionStmt(std::unique_ptr<BoolExpr> t_cond, std::unique_ptr<BlockStmt> t_body)
        : condition{std::move(t_cond)}, body{std::move(t_body)} {}

    std::unique_ptr<BoolExpr> condition;
    std::unique_ptr<BlockStmt> body;
};

class IfStmt : public Stmt {
public:
    explicit IfStmt(std::unique_ptr<ConditionStmt> t_condition)
        :ifStmt{std::move(t_condition)} {}

    void addElifStmt(std::unique_ptr<ConditionStmt> t_ifStmt) { elifStmts.emplace_back(std::move(t_ifStmt)); }
    void addElseBlock(std::unique_ptr<BlockStmt> t_blockStmt) { elseBody = std::move(t_blockStmt); }

    std::unique_ptr<ConditionStmt> ifStmt;
    std::vector<std::unique_ptr<ConditionStmt>> elifStmts;
    std::unique_ptr<BlockStmt> elseBody;
};

class Program {
public:
    std::vector<FnDeclStmt> functions;
};
