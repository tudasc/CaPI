//
// Created by ui72hona on 3/2/26.
//
#include "capi/nesmik_merger/NesmikMerger.h"

#include "LoggerUtil.h"
#include "io/MCGReader.h"
#include "io/VersionFourMCGWriter.h"

#include <fstream>
#include <iostream>

#define CXXOPTS_NO_RTTI
#include "cxxopts.hpp"

static void usage() {
    metacg::MCGLogger::logError(
            "Whoops!, try again with the correct files like: [<callgraph_in>.mcg] <talp>.json <nesmik>.json "
            "<callgraph_out>.mcg. The static input call graph (first parameter) is optional.");
}

int main(int argc, char** argv) {
    metacg::MCGLogger::logInfoUnique("Welcome to the neSmiK TALP callgraph merger!");

    cxxopts::Options options("prog", "Processes call graphs and JSON inputs");

    options.add_options()
            ("h,help", "Print usage")
            ("s,static-cg", "Static call graph file (optional)", cxxopts::value<std::string>()->default_value(""))
            ("t,talp-json", "TALP JSON file", cxxopts::value<std::string>())
            ("n,nesmik-json", "NeSmiK JSON file", cxxopts::value<std::string>())
            ("f,filter-json", "CaPI dynamic filter JSON file", cxxopts::value<std::string>()->default_value(""))
            ("o,output", "Output call graph file", cxxopts::value<std::string>());

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // Required args check
    if (!result.count("talp-json") || !result.count("nesmik-json") || !result.count("output")) {
        std::cerr << "Error: -t, -n, and -o are required.\n";
        std::cout << options.help() << std::endl;
        return 1;
    }

    bool have_static_cg = result.count("static-cg") > 0 &&
                          !result["static-cg"].as<std::string>().empty();

    bool have_filter_json = result.count("filter-json") > 0 &&
                            !result["filter-json"].as<std::string>().empty();

    std::string input_call_graph_file;
    std::string input_filter_file;
    std::string input_talp_json = result["talp-json"].as<std::string>();
    std::string input_nesmik_json = result["nesmik-json"].as<std::string>();
    std::string output_call_graph_file = result["output"].as<std::string>();

    if (have_static_cg) {
        input_call_graph_file = result["static-cg"].as<std::string>();
    }
    if (have_filter_json) {
        input_filter_file = result["filter-json"].as<std::string>();
    }

    auto static_cg = std::make_unique<metacg::Callgraph>();
    if (have_static_cg) {
        metacg::MCGLogger::logInfoUnique("Reading in static callgraph: {}", input_call_graph_file);
        metacg::io::FileSource fs(input_call_graph_file);
        auto mcgReader = metacg::io::createReader(fs);
        static_cg = std::move(mcgReader->read());
    }

    // Now read in the jsons

    metacg::MCGLogger::logInfoUnique("Reading in TALP json containing the performance metrics: {}", input_talp_json);
    std::ifstream talp_json_file(input_talp_json);
    nlohmann::json talp_json;
    talp_json_file >> talp_json;

    metacg::MCGLogger::logInfoUnique("Reading in the neSmiK json containing the region mappings: {}", input_nesmik_json);
    std::ifstream nesmik_json_file(input_nesmik_json);
    nlohmann::json nesmik_json;
    nesmik_json_file >> nesmik_json;

    std::unordered_set<std::string> dynamic_filter_list;
    if (have_filter_json) {
        std::ifstream filter_json_file(input_filter_file);
        nlohmann::json filter_json;
        filter_json_file >> filter_json;
        dynamic_filter_list = filter_json.get<std::unordered_set<std::string>>();
    }

    // do a quick sanity check in the jsons
    if (nesmik_json["_format"]["name"] != "capinesmik") {
        metacg::MCGLogger::logError("Expected valid json produced by neSmiK in file {}", input_nesmik_json);
        usage();
        std::exit(EXIT_FAILURE);
    }
    if (!talp_json.contains("dlbVersion")) {
        metacg::MCGLogger::logError("Expected valid json produced by TALP in file {}", input_talp_json);
        usage();
        std::exit(EXIT_FAILURE);
    }

    auto dynamic_cg = capi::buildDynamicGraphAndAttachMetrics(static_cg.get(), talp_json, nesmik_json, dynamic_filter_list);

    metacg::MCGLogger::logInfoUnique("Added metadata. Writing callgraph with TALP metadata into: {}",
                                     output_call_graph_file);

    auto mcgWriter = metacg::io::createWriter(4);
    metacg::io::JsonSink jsonSink;
    mcgWriter->write(dynamic_cg.get(), jsonSink);

    std::ofstream os(output_call_graph_file);
    os << jsonSink.getJson().dump(4) << std::endl;
}
