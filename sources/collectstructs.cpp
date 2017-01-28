#include "collectstructs.h"
#include "vvvptreehelper.hpp"
#include "myparamnames.hpp"

using namespace clang;

void addCommonFieldDecl(boost::property_tree::ptree& field,
                  const clang::FieldDecl* decl)
{
    using boost::property_tree::ptree;
    const auto& name = decl->getNameAsString();
    const auto& typestring = decl->getType().getAsString();
    const auto& comment = getComment((Decl*)decl);
    ptree_add_value(field, "field", name);
    ptree_add_value(field, "type", typestring);
    ptree_add_value(field, "comment", comment); 
}

void addArrayFieldDecl(boost::property_tree::ptree& field,
                       const clang::FieldDecl* decl)
{
    auto t = decl->getType();
    if(!t->isConstantArrayType())
        return;

    std::vector<uint64_t> arraySizeByDimensions;

    while(t->isConstantArrayType()) {
        ConstantArrayType* ca = (ConstantArrayType*)t->getAsArrayTypeUnsafe();
        const auto elemCount = *ca->getSize().getRawData();
        arraySizeByDimensions.push_back(elemCount);
        t = ca->getElementType();
    }

    const auto& elementTypeName = t.getAsString();
    
    using boost::property_tree::ptree;
    ptree arraySize;
    for(const auto& s: arraySizeByDimensions)
        ptree_array_add_values(arraySize, s);

    ptree arrayInfo;
    ptree_add_value(arrayInfo, "elemType", elementTypeName);
    ptree_add_subnode(arrayInfo, "size", arraySize);
    ptree_add_subnode(field, "array", arrayInfo);
}

void addBitfieldDecl(boost::property_tree::ptree& field,
                       const clang::FieldDecl* decl)
{
    if(!decl->isBitField())
        return;
    const auto& astctx = decl->getASTContext();
    const auto bitfieldWidth = decl->getBitWidthValue(astctx);
    using boost::property_tree::ptree;
    ptree_add_value(field, "bitfieldWidth", bitfieldWidth);
}

void addSizeIfBasic(boost::property_tree::ptree& field,
                       const clang::FieldDecl* decl)
{
    auto type = decl->getType();
    if(!type->isBuiltinType())
        return;

    using boost::property_tree::ptree;
    const auto& astctx = decl->getASTContext();
    ptree_add_value(field, "builtin", astctx.getTypeSize(type));
}

void printStructDecls(clang::ASTContext& Context,
                      boost::property_tree::ptree& tree,
                      const printStructsParam& params)
{
    using namespace std;
    using boost::property_tree::ptree;

    const auto declsInMain  = contain(params, PARAM_NAME_MAIN_ONLY)
                                         ? getMainFileDeclarations(Context)
                                         : getNonSystemDeclarations(Context);

    for(const auto& d: filterStructs(declsInMain)) {
        ptree fields;
        for(const auto& f: getStructFields(d)) {
            ptree field;
            addCommonFieldDecl(field, f);
            addArrayFieldDecl(field, f);
            addBitfieldDecl(field, f);
            addSizeIfBasic(field, f);
            ptree_array_add_node(fields, field);
        }

        ptree structdesc;
        ptree_add_value(structdesc, "name", getDeclName(d));
        ptree_add_value(structdesc, "comment", getComment((Decl*)d));
        if(!fields.empty())
            ptree_add_subnode(structdesc, "fields", fields);
        ptree_array_add_node(tree, structdesc);
    }
}

