#include "collectfunctions.h"
#include "myparamnames.hpp"
#include "vvvptreehelper.hpp"
#include <boost/algorithm/string.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>

using namespace clang;

std::string getDoxyBrief(const ASTContext& ctx, const RawComment* comment)
{
    return comment ? comment->getBriefText(ctx) : "";
}

/**
 * Return strings of doxygen comment without comment opening and closing.
 * From eahch line also removed decoration * from begining if exist */
std::vector<std::string> removeDecorations(const std::string& str)
{
    using namespace boost::algorithm;
    std::vector<std::string> ret;
    auto docstring = erase_tail_copy(erase_head_copy(str, 3), 2);
    split(ret, docstring, is_any_of("\n"), token_compress_on);
    std::transform(ret.begin(), ret.end(), ret.begin(), [](const auto& str) {
        return boost::algorithm::trim_copy(str);
    });
    std::transform(ret.begin(), ret.end(), ret.begin(), [](const auto& str) {
        return str[0] == '*' ? erase_head_copy(str, 1) : str;
    });
    return ret;
}

std::vector<std::string> splitToTrimmedWords(const std::string& str)
{
    using namespace boost::algorithm;
    std::vector<std::string> splited;

    split(splited, str, is_any_of(" \t"), token_compress_on);
    return filter(splited, [](const auto& str) { return !str.empty(); });
}

/**
 * Function join elements from 'c' using sep omitting n first elements.
 * @param c container
 * @param n number of head elements to exclude
 * @param sep separator to insert
 */
template <class T, typename S>
auto joinTail(const T& c, size_t n, const S& sep)
{
    const auto& tail    = std::vector<std::string>(c.begin() + n, c.end());
    const auto& comment = boost::algorithm::join(tail, sep);
    return comment;
}

std::map<std::string, std::string> getDoxyParams(const ASTContext& ctx,
                                                 const RawComment* rawcomment)
{
    std::map<std::string, std::string> ret;
    if (rawcomment == nullptr)
        return ret;

    const SourceManager& sm = ctx.getSourceManager();
    const auto& text        = rawcomment->getRawText(sm).str();
    const auto& lines       = removeDecorations(text);

    for (const auto& l : lines) {
        static const auto paramTags  = {"@param", "\\param"};
        static const auto returnTags = {"@return", "\\return"};

        const auto words         = splitToTrimmedWords(l);
        const size_t splitedsize = words.size();
        if (splitedsize < 2)
            continue;

        const auto& Tag = words[0];
        if (contain(paramTags, Tag) && splitedsize > 2) {
            const auto& paramName = words[1];
            const auto& comment   = joinTail(words, 2, " ");
            ret[paramName]        = comment;
        }
        else if (contain(returnTags, Tag)) {
            const auto& comment = joinTail(words, 1, " ");
            ret["return"]       = comment;
        }
    }

    return ret;
}

boost::property_tree::ptree
makeFunctionDescriptionNode(const clang::FunctionDecl* d, bool needSources)
{
    using namespace std;
    using boost::property_tree::ptree;
    const clang::ASTContext& Context = d->getASTContext();

    ptree params;
    const RawComment* rc = Context.getRawCommentForDeclNoCache(d);
    const auto& brief    = getDoxyBrief(Context, rc);
    auto paramsComments  = getDoxyParams(Context, rc);
    for (const auto& f : getFunctionParams(d)) {
        const auto& name = f->getNameAsString();
        const auto& t    = f->getType().getAsString();
        const std::string& comment =
            paramsComments.count(name) ? paramsComments[name] : "";
        ptree param;
        ptree_add_value(param, "name", name);
        ptree_add_value(param, "type", t);
        ptree_add_value(param, "comment", comment);
        ptree_array_add_node(params, param);
    }

    const auto& sm = d->getASTContext().getSourceManager();
    const auto& locationstring = d->getLocation().printToString(sm);

    ptree functiondesc;
    ptree_add_value(functiondesc, "location", locationstring);
    ptree_add_value(functiondesc, "name", getDeclName(d));
    ptree_add_value(functiondesc, "rettype", d->getReturnType().getAsString());
    ptree_add_value(functiondesc, "retcomment", paramsComments["return"]);
    ptree_add_value(functiondesc, "comment", brief);
    ptree_add_subnode(functiondesc, "params", params);
    ptree_add_value(functiondesc, "source", needSources ? decl2str(d) : "");
    return functiondesc;
}

void printFunctionDecls(clang::ASTContext& Context,
                        boost::property_tree::ptree& tree,
                        const printFunctionsParam& params)
{
    using namespace std;
    using boost::property_tree::ptree;

    const auto declsInMain = contain(params, PARAM_NAME_MAIN_ONLY)
                                 ? getMainFileDeclarations(Context)
                                 : getNonSystemDeclarations(Context);
    const auto needSources = contain(params, PARAM_NAME_WITH_SOURCE);
    for (const auto& d : filterFunctions(declsInMain)) {
        const ptree functiondesc = makeFunctionDescriptionNode(d, needSources);
        ptree_array_add_node(tree, functiondesc);
    }
}
