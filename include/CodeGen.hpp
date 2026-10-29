#pragma once

#include "Ast.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include <memory>
#include <string>
#include <unordered_map>


class CodeGen {
public:
    explicit CodeGen(const Program&);
    void print() const;
private:
    llvm::Type* getLlvmType(Types) const;

    llvm::Function* generateFunction(const FnDeclStmt&);
    void generateBlock(const BlockStmt&);
    void generateStatement(const Stmt&);
    llvm::Value* generateExpr(const Expr&);

    std::unique_ptr<llvm::LLVMContext> TheContext;
    std::unique_ptr<llvm::Module> TheModule;
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::unordered_map<std::string, llvm::AllocaInst*> NamedValues;
};
