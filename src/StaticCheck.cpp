#include "StaticCheck.hpp"

#include <format>
#include <print>

using namespace std::string_view_literals;

Result Scope::addNewFunction(const std::string& t_fnName, const FunctionInfo& t_fnInfo) {
    if (const auto it = functions_.find(t_fnName); it != functions_.end()) {
        return Result{"Function redeclaration."};
    }

    functions_.emplace(t_fnName, t_fnInfo);
    currentFunctionName_ = t_fnName;

    varScope_ = {};
    vars_.clear();

    return Result{};
}

std::expected<const FunctionInfo*, Result> Scope::getFunctionInfo(const std::string& t_fnName) const {
    std::string_view functionName = t_fnName;
    if (t_fnName.empty()) {
        functionName = currentFunctionName_;
    }

    if(const auto it = functions_.find(functionName); it != functions_.end()) {
        return &it->second;
    }

    return std::unexpected{Result{"Function is not declared."}};
}

void Scope::popFunction() {
    currentFunctionName_ = "";
}

void Scope::popBlock() {
    const auto& scopeVariables = varScope_.top();

    for (const auto& [name, _]: scopeVariables) {
        vars_.erase(name);
    }

    varScope_.pop();
}

void Scope::pushBlock() {
    varScope_.push({});
}

Result Scope::addVariable(const std::string_view t_name, const Types t_type) {
    const auto fn = this->getFunctionInfo();
    if (not fn.has_value()) {
        return Result{"There is no present function were to declare a variable: {}", fn.error().error};
    }

    const auto& parameters = fn.value()->parameters;
    if (const auto it = std::ranges::find(parameters, t_name, &VariableInfo::name); it != parameters.end()) {
        return Result{"Variable defined as function parameter: {}", t_name};
    }

    const auto temp = vars_ | std::ranges::to<std::vector<std::pair<std::string_view, Types>>>();

    if (vars_.contains(t_name)) {
        throw Result{"Variable already defined inside previous scope."};
    }

    vars_.insert({t_name, t_type});

    auto& topScope = varScope_.top();
    topScope.emplace_back(std::string{t_name}, t_type);

    return Result{};
}

bool Scope::containsVariable(const std::string_view t_varName) {
    const auto fnRez = this->getFunctionInfo();
    if (not fnRez.has_value()) {
        return false;
    }

    const auto& fn = fnRez.value();
    const auto& parameters = fn->parameters;

    if (const auto it = std::ranges::find(parameters, t_varName, &VariableInfo::name); it != parameters.end()) {
        return true;
    }

    const auto it = vars_.find(t_varName);
    return it != vars_.end();
}

std::expected<VariableInfo, Result> Scope::findVariable(const std::string_view t_varName) {
    const auto fnRez = this->getFunctionInfo();
    if (not fnRez.has_value()) {
        return std::unexpected{Result{"Variable can't be declared in global scope: {}", fnRez.error().error}};
    }

    const auto& fn = fnRez.value();
    const auto& parameters = fn->parameters;

    if (const auto it = std::ranges::find(parameters, t_varName, &VariableInfo::name); it != parameters.end()) {
        return VariableInfo{it->name, it->type};
    }

    if (const auto it = vars_.find(t_varName); it != vars_.end()) {
        return VariableInfo{std::string{t_varName}, it->second};
    }

    return std::unexpected{Result{"Variable already defined in the current scope."}};
}

StaticCheck::StaticCheck(): scope_{} {}

std::expected<Types, Result> StaticCheck::getExpressionType(const Expr* t_expr) {
    if (const auto literalExpr = dynamic_cast<const LiteralExpr*>(t_expr)) {
        return literalExpr->literalType;
    }

    if (const auto identityExpr = dynamic_cast<const IdentExpr*>(t_expr)) {
        const auto varInfoRez = scope_.findVariable(identityExpr->identName);

        if (not varInfoRez.has_value()) {
            return std::unexpected{Result{"Undefined variable in use."}};
        }

        return varInfoRez.value().type;
    }

    if (const auto callExpr = dynamic_cast<const CallExpr*>(t_expr)) {
        const auto fnRez = scope_.getFunctionInfo(callExpr->functionName);
        if (not fnRez.has_value()) {
            return std::unexpected{Result{"Unknown function call"}};
        }

        const auto fnParameters = fnRez.value()->parameters;
        const auto& parameters = callExpr->args;

        if (fnParameters.size() != parameters.size()) {
            return std::unexpected{Result{"Mismatch between function parameter count and passed parameter count."}};
        }

        for (auto i{0}; i < fnParameters.size(); ++i) {
            const auto& par = parameters[i];
            const auto parameterType = getExpressionType(par.get());

            if (parameterType != fnParameters[i].type) {
                return std::unexpected{Result{"Function parameter type mismatch."}};
            }
        }

        const auto fnInfo = fnRez.value();
        return fnInfo->returnType;
    }

    return std::unexpected{Result{"Unknown expression type."}};
}

Result StaticCheck::validateBooleanExpression(const BoolExpr& t_boolExpr) {
    if (const auto* boolExpr = dynamic_cast<const BoolLiteral*>(&t_boolExpr)) {
        return Result{};
    }

    if (const auto* cmpBool = dynamic_cast<const ComparingBoolExpr*>(&t_boolExpr)) {
        const auto leftSideTypeRez = getExpressionType(cmpBool->lh.get());
        if (not leftSideTypeRez.has_value()) {
            return leftSideTypeRez.error();
        }

        const auto rightSideTypeRez = getExpressionType(cmpBool->rh.get());
        if (not rightSideTypeRez.has_value()) {
            return rightSideTypeRez.error();
        }

        if (rightSideTypeRez.value() != rightSideTypeRez.value()) {
            return Result{"Comparison between expression type mismatch."};
        }

        return Result{};
    }

    std::unreachable();
}

Result StaticCheck::validateVariableDeclaration(const VarDeclStmt& t_varDecl) {
    const auto varAlreadyDecl = scope_.containsVariable(t_varDecl.identifierName);
    if (varAlreadyDecl) {
        return Result{"Variable already declared."};
    }

    scope_.addVariable(t_varDecl.identifierName, t_varDecl.varTypeStmt->type);
    return Result{};
}

Result StaticCheck::validateVariableInitialization(const VarInitStmt& t_varInit) {
    const auto& varDecl =t_varInit.varDecl.get();

    const auto varDeclRez = validateVariableDeclaration(*varDecl);
    if (varDeclRez.hasError()) {
        return varDeclRez;
    }

    const auto exprTypeRez = getExpressionType(t_varInit.initializer.get());
    if (not exprTypeRez.has_value()) {
        return exprTypeRez.error();
    }

    const auto exprType = exprTypeRez.value();
    if (exprType != t_varInit.varDecl->varTypeStmt->type) {
        return Result{"Mismatch between variables type and assigned value."};
    }

    return Result{};
}

Result StaticCheck::validateAssignment(const VarAssignStmt& t_varAssign) {
    const auto variableInfoRez = scope_.findVariable(t_varAssign.name);

    if (not variableInfoRez.has_value()) {
        return Result{"Variable not declared: {}", t_varAssign.name};
    }

    const auto& varInfo = variableInfoRez.value();

    const auto exprTypeRez = getExpressionType(t_varAssign.assignment.get());
    if (not exprTypeRez.has_value()) {
        return exprTypeRez.error();
    }

    const auto& exprType = exprTypeRez.value();
    if (varInfo.type != exprType) {
        return Result("Assignment type mismatch.");
    }

    return Result{};
}

Result StaticCheck::validateReturnStatement(const FunctionInfo& t_fn, const ReturnStmt& t_return) {
    const auto exprTypeRez = getExpressionType(t_return.returnExpr.get());
    if (not exprTypeRez.has_value()) {
        return exprTypeRez.error();
    }

    const auto& exprType = exprTypeRez.value();
    if (t_fn.returnType != exprType) {
        return Result("Function return type mismatch.");
    }

    return Result{};
}

Result StaticCheck::validateBlock(const BlockStmt& t_block) {
    scope_.pushBlock();

    for (const auto& stmt : t_block.statements) {
        if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(stmt.get())) {
            const auto fnRez = scope_.getFunctionInfo();
            if (not fnRez.has_value()) {
                throw std::runtime_error{"Blet eik naxui"};
            }

            const auto fn = fnRez.value();
            const auto returnRez = validateReturnStatement(*fn, *returnStmt);
            if (returnRez.hasError()) {
                return returnRez;
            }
            continue;
        }

        //what to check
        if (const auto *varTypeStmt = dynamic_cast<const VarTypeStmt*>(stmt.get())) {
            continue;
        }

        if (const auto* varDeclStmt = dynamic_cast<const VarDeclStmt*>(stmt.get())) {
            const auto varDeclRez = validateVariableDeclaration(*varDeclStmt);
            if (varDeclRez.hasError()) {
                return varDeclRez;
            }
            continue;
        }

        if (const auto* varInitStmt = dynamic_cast<const VarInitStmt*>(stmt.get())) {
            const auto varInitRez = validateVariableInitialization(*varInitStmt);
            if (varInitRez.hasError()) {
                return varInitRez;
            }
            continue;
        }

        if (const auto* varAssignStmt = dynamic_cast<const VarAssignStmt*>(stmt.get())) {
            const auto varAssRez = validateAssignment(*varAssignStmt);
            if (varAssRez.hasError()) {
                return varAssRez;
            }
            continue;
        }

        if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt.get())) {
            const auto blockRez = validateBlock(*blockStmt);
            if (blockRez.hasError()) {
                return blockRez;
            }
            continue;
        }

        if (const auto* condStmt = dynamic_cast<const IfStmt*>(stmt.get())) {
            const auto condRez = validateConditions(*condStmt);
            if (condRez.hasError()) {
                return condRez;
            }
            continue;
        }

        return Result{"Unknown statement."};
    }

    scope_.popBlock();
    return Result{};
}

Result StaticCheck::validateFunction(const FnDeclStmt& t_fn) {
    std::vector<VariableInfo> parameterInfo;
    parameterInfo.reserve(t_fn.parameters.size());

    for (const auto& param : t_fn.parameters) {
        const auto& paramName = param->identifierName;
        const auto& paramType = param->varTypeStmt->type;

        if (paramType == Types::Void) {
            return Result{"Parameter type can't be void."};
        }

        if (std::ranges::find(parameterInfo, paramName, &VariableInfo::name) != parameterInfo.end()) {
            return Result{"Duplicate parameter declaration."};
        }

        parameterInfo.emplace_back(paramName, paramType);
    }

    const auto functionInfo = FunctionInfo{parameterInfo, t_fn.returnType->type};

    const auto fnRez = scope_.addNewFunction(t_fn.functionName, functionInfo);
    if (not fnRez.error.empty()) {
        return fnRez;
    }

    const auto blockRez = validateBlock(*t_fn.body);
    if (blockRez.hasError()) {
        return blockRez;
    }

    scope_.popFunction();

    return Result{};
}

Result StaticCheck::validateIfStatement(const ConditionStmt& t_if) {
    const auto boolRez = validateBooleanExpression(*t_if.condition);
    if (boolRez.hasError()) {
        return boolRez;
    }

    const auto blockRez = validateBlock(*t_if.body);
    return blockRez;
}

Result StaticCheck::validateConditions(const IfStmt& t_cond) {
    const auto ifRez = validateIfStatement(*t_cond.ifStmt);
    if (ifRez.hasError()) {
        return ifRez;
    }

    for (const auto& elifStmt : t_cond.elifStmts) {
        const auto elifRez = validateIfStatement(*elifStmt);
        if (elifRez.hasError()) {
            return elifRez;
        }
    }

    if (t_cond.elseBody != nullptr) {
        const auto elseRez = validateBlock(*t_cond.elseBody);
        if (elseRez.hasError()) {
            return elseRez;
        }
    }

    return Result{};
}

bool StaticCheck::staticAnalysis(const Program& t_program) {
    for (const auto& fn : t_program.functions) {
        const auto fnRez = validateFunction(fn);
        if (fnRez.hasError()) {
            std::println("Function verification failed: {}", fnRez.error);
            return false;
        }
    }

    return true;
}