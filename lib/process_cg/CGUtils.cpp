//
// Created by sebastian on 10.11.21.
//

#include "CallGraph.h"
#include "MetaCGReader.h"
#include "Selectors.h"
#include "DOTWriter.h"

#include <iostream>
#include <fstream>


// erase_if for maps is only available in C++20
template< typename ContainerT, typename PredicateT >
void erase_if( ContainerT& items, const PredicateT& predicate ) {
    for( auto it = items.begin(); it != items.end(); ) {
        if( predicate(*it) ) it = items.erase(it);
        else ++it;
    }
}


int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Missing input file argument\n";
        return 1;
    }

    std::string infile = argv[1];
    std::string targetFn = argv[2];

//    for (int i = 1; i < argc; i++) {
//        std::string arg = argv[i];
//        if (arg.length() > 2 && arg[0] == '-' && arg[1] == '-') {
//            auto option = arg.substr(2);
//            if (option.compare("write-dot") == 0) {
//                shouldWriteDOT = true;
//            }
//            continue;
//        }
//        infile = arg;
//    }

    std::cout << "Loading CG from " << infile << "\n";

    MetaCGReader reader(infile);
    if (!reader.read()) {
        std::cerr << "Unable to load call graph\n";
        return 1;
    }

    auto functionInfo = reader.getFunctionInfo();

    auto cg = createCG(functionInfo);

    std::cout << "Loaded CG with " << cg->size() << " nodes\n";

    auto targetNode = cg->get(targetFn);

    if (!targetNode) {
        std::cerr << "Function" << targetFn << " could not be found.\n";
        return 1;
    }

    SelectorRunner runner(*cg);
    auto selector = selector::onCallPathFrom(selector::byName(targetFn, selector::all()));
    auto result = runner.run(*selector);

    std::cout << "Selected " << result.size() << " functions.\n";

    auto filteredFunctionInfo = functionInfo;
    erase_if(filteredFunctionInfo, [&result](const auto& entry) -> auto {
        return std::find(result.begin(), result.end(), entry.first) == result.end();
    });

    auto filteredCG = createCG(filteredFunctionInfo);

    std::cout << "Filtered CG size: " << filteredCG->size() << "\n";

    std::string dotfile = "cg_" + targetFn + ".dot";
    {
        std::ofstream os(dotfile);
        if (os.is_open()) {
            writeDOT(*filteredCG, os);
        }
    }

    return 0;
}