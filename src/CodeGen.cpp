#include "CodeGen.hpp"

#include <charconv>

#include "llvm/Support/raw_ostream.h"

#include <vector>

CodeGen::CodeGen(const Program& t_program)
    : TheContext{std::make_unique<llvm::LLVMContext>()},
      TheModule{std::make_unique<llvm::Module>("My module", *TheContext)},
      Builder{std::make_unique<llvm::IRBuilder<>>(*TheContext)} {

    for (const auto& function : t_program.functions) {
        generateFunction(function);
    }
}

void CodeGen::print() const {
    TheModule->print(llvm::outs(), nullptr);
}

llvm::Type* CodeGen::getLlvmType(const Types t_type) const {
    switch (t_type) {
        case Types::I32:  return llvm::Type::getInt32Ty(*TheContext);
        case Types::Void: return llvm::Type::getVoidTy(*TheContext);
    }
    return nullptr;
}

llvm::Function* CodeGen::generateFunction(const FnDeclStmt& t_fnDecl) {
    std::vector<llvm::Type*> parameterTypes;
    parameterTypes.reserve(t_fnDecl.parameters.size());

    for (const auto& parameter : t_fnDecl.parameters) {
        const auto type = getLlvmType(parameter->varTypeStmt->type);
        parameterTypes.push_back(type);
    }

    auto* returnType = getLlvmType(t_fnDecl.returnType->type);
    auto* functionType = llvm::FunctionType::get(returnType, parameterTypes, false);
    auto* function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage,
                                            t_fnDecl.functionName, *TheModule);

    for (std::size_t i{}; i < t_fnDecl.parameters.size(); ++i) {
        function->getArg(i)->setName(t_fnDecl.parameters[i]->identifierName);
    }

    // 1. entry block FIRST, and point the builder at it
    auto* entry = llvm::BasicBlock::Create(*TheContext, "entry", function);
    Builder->SetInsertPoint(entry);

    // 2. spill parameters
    NamedValues.clear();
    for (auto& arg : function->args()) {
        auto* slot = Builder->CreateAlloca(arg.getType(), nullptr, arg.getName());

        Builder->CreateStore(&arg, slot);
        NamedValues.insert({std::string(arg.getName()), slot});
    }

    generateBlock(*t_fnDecl.body);

    if (!Builder->GetInsertBlock()->getTerminator()) {
        if (returnType->isVoidTy()) Builder->CreateRetVoid();
        else Builder->CreateRet(llvm::Constant::getNullValue(returnType));
    }

    llvm::verifyFunction(*function, &llvm::errs());
    return function;
}

void CodeGen::generateBlock(const BlockStmt& t_block) {
    for (const auto& stmt : t_block.statements) {
        generateStatement(*stmt);
        if (Builder->GetInsertBlock()->getTerminator()) {
            return;
        }
    }
}

void CodeGen::generateStatement(const Stmt& s) {
    if (auto* ret = dynamic_cast<const ReturnStmt*>(&s)) {
        if (ret->returnExpr != nullptr) {
            const auto exprValue = generateExpr(*ret->returnExpr);

            Builder->CreateRet(exprValue);
        }
        else {
            Builder->CreateRetVoid();
        }
    }
}

llvm::Value* CodeGen::generateExpr(const Expr& e) {
    if (const auto* v = dynamic_cast<const IdentExpr*>(&e)) {
        const auto it = NamedValues.find(v->identName);

        if (it == NamedValues.end()) {
            return nullptr;
        }

        auto* slot = it->second;
        return Builder->CreateLoad(slot->getAllocatedType(), slot, v->identName);
    }

    if (const auto* n = dynamic_cast<const LiteralExpr*>(&e)) {
        const auto [type, value] = *n;
        if (n->literalType == Types::I32) {
            auto number{0};
            const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), number);

            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), number, false);
            //return llvm::ConstantInt::get(*TheContext, number);
        }
    }

    if (const auto* f = dynamic_cast<const CallExpr*>(&e)) {
        llvm::Function* fn = TheModule->getFunction(f->functionName);
        if (fn == nullptr) {
            return nullptr;
        }

        if (fn->arg_size() != f->args.size()) {
            return nullptr;
        }

        std::vector<llvm::Value*> args;
        args.reserve(fn->arg_size());

        for (const auto& arg : f->args) {
            auto* expr = generateExpr(*arg);

            if (expr == nullptr) {
                return nullptr;
            }

            args.push_back(expr);
        }

        return Builder->CreateCall(fn, args);
    }

    return nullptr;
}