#pragma once

#include "Parser.hpp"
#include "Utils.hpp"

#include <stack>
#include <unordered_map>
#include <string>
#include <expected>
#include <format>

struct Result {
    Result() = default;
    template <typename... Args> requires (std::is_convertible_v<std::decay_t<Args>, std::string_view> && ...)
    explicit Result(std::format_string<Args...> t_fmt, Args&&... t_args) {
        error = std::format(t_fmt, std::forward<Args>(t_args)...);
    }
    std::string error;

    bool ok() const {
        return error.empty();
    }

    bool hasError() const {
        return not ok();
    }
};

struct VariableInfo {
    std::string name;
    Types type;
};

struct FunctionInfo {
    FunctionInfo(const std::vector<VariableInfo>& t_params, const Types t_type): parameters{t_params}, returnType{t_type} {}

    std::vector<VariableInfo> parameters;
    Types returnType;
};

class Scope {
public:
    explicit Scope() = default;

    [[nodiscard]] Result addNewFunction(const std::string& t_fnName, const FunctionInfo& t_fnInfo);
    void popFunction();
    [[nodiscard]] std::expected<const FunctionInfo*, Result> getFunctionInfo(const std::string& t_fnName = "") const;

    void popBlock();
    void pushBlock();

    Result addVariable(std::string_view t_name, const Types t_type);
    [[nodiscard]] bool containsVariable(std::string_view t_name);
    [[nodiscard]] std::expected<VariableInfo, Result> findVariable(std::string_view t_name);
private:
    std::unordered_map<std::string_view, FunctionInfo> functions_;
    std::string_view currentFunctionName_;

    std::unordered_map<std::string_view, Types> vars_;
    std::stack<std::vector<VariableInfo>> varScope_;
};

class StaticCheck {
public:
    StaticCheck();
    bool staticAnalysis(const Program&);
private:
    Scope scope_;

    std::expected<Types, Result> getExpressionType(const Expr*);
    Result validateBooleanExpression(const BoolExpr&);

    Result validateVariableDeclaration(const VarDeclStmt&);
    Result validateVariableInitialization(const VarInitStmt&);
    Result validateAssignment(const VarAssignStmt&);
    Result validateReturnStatement(const FunctionInfo&, const ReturnStmt&);

    Result validateBlock(const BlockStmt&);
    Result validateFunction(const FnDeclStmt&);

    Result validateConditions(const IfStmt&);
    Result validateIfStatement(const ConditionStmt&);
};