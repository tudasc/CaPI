//
// Created by sebastian on 28.10.21.
//

#include "CaPI.h"

#include <string>
#include <map>
#include <iostream>
#include <fstream>

#include "MetaCGReader.h"
#include "CallGraph.h"
#include "DOTWriter.h"
#include "Selectors.h"
#include "FunctionFilter.h"
#include "SelectionSpecLoader.h"

void printHelp() {
    std::cout << "Usage: capi [options] cg_file\n";
    std::cout << "Options:\n";
    std::cout << " -p <preset>  Use a selection preset, where <preset> is one of 'MPI','FOAM'.}\n";
    std::cout << " -f <file>    Use a selection spec file.\n";
    std::cout << " --write-dot  Write a dotfile of the selected call-graph subset.\n";
}


enum class SelectionPreset {
    MPI, OPENFOAM_MPI,
};

namespace {
    std::map<std::string, SelectionPreset> presetNames = {{"MPI", SelectionPreset::MPI}, {"FOAM", SelectionPreset::OPENFOAM_MPI}};
}


int main(int argc, char** argv) {

    if (argc < 3) {
        std::cerr << "Missing input file argument\n";
        return 1;
    }

    bool shouldWriteDOT{false};
    std::string cgfile, specfile;
    SelectionPreset preset;
    bool usePreset{false};

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.length() > 1 && arg[0] == '-') {
            if (arg.length() > 2 && arg[1] == '-') {
                auto option = arg.substr(2);
                if (option.compare("write-dot") == 0) {
                    shouldWriteDOT = true;
                }
            } else {
                auto option = arg.substr(1);
                if (option.compare("p") == 0) {
                    if (++i >= argc) {
                        std::cerr << "Need to pass a selection preset after -p\n";
                        printHelp();
                        return EXIT_FAILURE;
                    }
                    auto presetStr = argv[i];
                    if (auto it = presetNames.find(presetStr); it != presetNames.end()) {
                        usePreset = true;
                        preset = it->second;
                    }

                } else if (option.compare("f") == 0) {
                    if (++i >= argc) {
                        std::cerr << "Need to pass a selection spec file after -f\n";
                        printHelp();
                        return EXIT_FAILURE;
                    }
                    specfile = argv[i];
                }
            }
            continue;
        }
        if (cgfile.empty()) {
            cgfile = arg;
            continue;
        }
    }

    if (!usePreset && specfile.empty()) {
        std::cerr << "Need to specify either a preset or pass a selection file.\n";
        printHelp();
    }

    SelectorPtr selector;
    if (usePreset) {
        switch(preset) {
            case SelectionPreset::MPI:
                selector = selector::onCallPathTo(selector::byName("MPI_.*", selector::all()));
            case SelectionPreset::OPENFOAM_MPI:
                selector = selector::subtract(selector::onCallPathTo(selector::byName("MPI_.*", selector::all())),
                                              selector::byPath(".*\\/OpenFOAM\\/db\\/.*", selector::all()));
            default:
                assert(false && "Preset not implemented");
        }
    } else {
        SelectionSpecLoader specLoader(specfile);
        selector = specLoader.buildSelector();
        if (!selector) {
            std::cerr << "Unable to load selection specs.\n";
            return EXIT_FAILURE;
        }
    }

    std::cout << "Loading call graph from " << cgfile << "\n";

    MetaCGReader reader(cgfile);
    if (!reader.read()) {
        std::cerr << "Unable to load call graph!\n";
        return EXIT_FAILURE;
    }

    auto functionInfo = reader.getFunctionInfo();

    auto cg = createCG(functionInfo);

    std::cout << "Loaded CG with " << cg->size() << " nodes\n";


    SelectorRunner runner(*cg);
    auto result = runner.run(*selector);

    std::cout << "Selected " << result.size() << " functions.\n";

    auto outfile = cgfile.substr(0, cgfile.find_last_of('.')) + ".filt";

    {
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

    return EXIT_SUCCESS;
}
