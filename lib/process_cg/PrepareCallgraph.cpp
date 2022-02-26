//
// Created by sebastian on 28.10.21.
//

#include "PrepareCallgraph.h"

#include <string>
#include <iostream>
#include <fstream>

#include "MetaCGReader.h"
#include "CallGraph.h"
#include "DOTWriter.h"
#include "Selectors.h"
#include "FunctionFilter.h"
#include "SelectionSpecLoader.h"

int main(int argc, char** argv) {

    if (argc < 3) {
        std::cerr << "Missing input file argument\n";
        return 1;
    }

    bool shouldWriteDOT = false;
    std::string cgfile, specfile;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.length() > 2 && arg[0] == '-' && arg[1] == '-') {
            auto option = arg.substr(2);
            if (option.compare("write-dot") == 0) {
                shouldWriteDOT = true;
            }
            continue;
        }
        if (cgfile.empty()) {
            cgfile = arg;
            continue;
        }
        specfile = arg;
    }

//
//    std::cout << "Loading selection specs from " << cgfile << "\n";
//
//    SelectionSpecLoader specLoader(specfile);
//
//    auto selector = specLoader.buildSelector();

    std::cout << "Loading CG from " << cgfile << "\n";

    MetaCGReader reader(cgfile);
    if (!reader.read()) {
        std::cerr << "Unable to load call graph\n";
        return 1;
    }

    auto functionInfo = reader.getFunctionInfo();

    auto cg = createCG(functionInfo);

    std::cout << "Loaded CG with " << cg->size() << " nodes\n";


    SelectorRunner runner(*cg);

    auto selector = selector::subtract(selector::join(selector::onCallPathTo(selector::containingUnresolvedCall(selector::all())), selector::onCallPathTo(selector::byName("MPI_.*", selector::all()))),
                                       selector::byPath(".*\\/OpenFOAM\\/db\\/.*", selector::all()));

    auto result = runner.run(*selector);

    std::cout << "Selected " << result.size() << " functions.\n";

    auto outfile = cgfile.substr(0, cgfile.find_last_of('.')) + ".filt";

    {
//        std::ofstream os(outfile);
//        for (auto& f : result) {
//           os << f << "\n";
//        }
        FunctionFilter filter;
        for (auto& f : result) {
            filter.addIncludedFunction(f);
        }
        if (!writeScorePFilterFile(filter, outfile)) {
            std::cerr << "Error: Writing filter file failed.\n";
        }
    }

    if (shouldWriteDOT) {
        std::ofstream os("cg.dot");
        if (os.is_open()) {
            writeDOT(*cg, os);
        }
    }

    return 0;
}
