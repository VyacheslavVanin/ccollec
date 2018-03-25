#pragma once
#include <boost/property_tree/ptree.hpp>

#include <clang/Analysis/AnalysisDeclContext.h>

#include "clanghelper/stdhelper/containerhelper.hpp"
#include "clanghelper/vvvclanghelper.hpp"
#include "myparamnames.hpp"
#include "collectfunctions.hpp"
#include "collectstructs.hpp"

using ParamList = std::vector<std::string>;

class ExtractDataConsumer : public clang::ASTConsumer {
public:
    explicit ExtractDataConsumer(clang::ASTContext* Context,
                                 boost::property_tree::ptree& tree,
                                 const ParamList& params)
        : tree(tree), params(params)
    {
    }

    ~ExtractDataConsumer() {
        tree.push_back(std::make_pair("structs", structdescs));
        tree.push_back(std::make_pair("functions", functiondescs));
    }

    virtual void HandleTranslationUnit(clang::ASTContext& Context)
    {
        using namespace vvv::helpers;
        const auto needStructs = !contain(params, PARAM_NAME_NO_STRUCTS);
        const auto needFunctions = !contain(params, PARAM_NAME_NO_FUNCS);
        const auto declsInMain = contain(params, PARAM_NAME_MAIN_ONLY)
                                     ? getMainFileDeclarations(Context)
                                     : getNonSystemDeclarations(Context);
        if (needFunctions)
            printFunctionDecls(declsInMain, functiondescs, params);
        if (needStructs)
            printStructDecls(declsInMain, structdescs, params);
    }

    boost::property_tree::ptree& tree;
    const ParamList& params;
    boost::property_tree::ptree structdescs;
    boost::property_tree::ptree functiondescs;
};

class CollectDeclsAction : public clang::ASTFrontendAction {
public:
    CollectDeclsAction(boost::property_tree::ptree& tree,
                       const ParamList& params)
        : tree(tree), params(params)
    {
    }

    virtual std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance& Compiler, llvm::StringRef InFile)
    {
        return std::unique_ptr<clang::ASTConsumer>(
            new ExtractDataConsumer(&Compiler.getASTContext(), tree, params));
    }

    boost::property_tree::ptree& tree;
    const ParamList& params;
};
