#include <set>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "collectstructs.h"
#include "collectfunctions.h"
#include "arghelper.hpp"
#include "myparamnames.hpp"

void printHelpIfNeeded(const std::vector<std::string>& params)
{
    static const char* message = 
    "cstructinfo usage:\n"
    "cstructinfo <input files ...> [clang compiler options] [options]\n"
    "Options:\n"
    "--main-only - do not process any included files. Process only specified\n"
    "--no-functions - exclude functions descriptions from output\n"
    "--no-structs - exclude structs descriptions from output\n"
    "--no-sizes - do not add sizeofs of primitive types to structs descriptions\n"
    "--help - show this help\n";
    if(params.empty() || contain(params, PARAM_NAME_HELP)){
        std::cout << message << std::endl;
        exit(EXIT_SUCCESS);
    }
    return;
}

int StructAndFuncInfoCollector(int argc, char** argv)
{
    if(argc < 2) { std::cerr << "fatal error: no input files"<< std::endl;}

    static const std::set<std::string> myParameters = {PARAM_NAME_MAIN_ONLY, 
                                                       PARAM_NAME_NO_FUNCS,
                                                       PARAM_NAME_NO_STRUCTS,
                                                       PARAM_NAME_NO_SIZES,
                                                       PARAM_NAME_HELP};
    static const auto myParamFilter = [](const auto& p)
                                {return contain(myParameters, p);};
    static const auto notMyParamsFilter = [](const auto& p)
                                {return !myParamFilter(p);};

    boost::property_tree::ptree root;
    boost::property_tree::ptree structdescs;
    boost::property_tree::ptree functiondescs;

    const auto args = argstoarray(argc, argv);
    const auto allParams    = filterParams(args);
    const auto myParams     = filter(allParams, myParamFilter);
    const auto cxxparams    = filter(allParams, notMyParamsFilter);
    const auto filenames    = filterNotParams(args);

    const auto needStructs   = !contain(myParams, PARAM_NAME_NO_STRUCTS);
    const auto needFunctions = !contain(myParams, PARAM_NAME_NO_FUNCS);

    printHelpIfNeeded(args);

    for( const auto& name: filenames) {
        using namespace clang::tooling;
        const auto code = getSourceFromFile( name.c_str() );
        if(needStructs)
            runToolOnCodeWithArgs( 
                    new CollectStructsInfoAction(structdescs, myParams),
                    code.c_str(), cxxparams, name.c_str() );
        if(needFunctions)
            runToolOnCodeWithArgs(
                    new CollectFunctionsInfoAction(functiondescs, myParams),
                    code.c_str(), cxxparams, name.c_str() );
    }

    if(structdescs.size())
        root.push_back(std::make_pair("structs",structdescs));
    if(functiondescs.size())
        root.push_back(std::make_pair("functions",functiondescs));
    boost::property_tree::write_json(std::cout, root);
    return 0;
}

