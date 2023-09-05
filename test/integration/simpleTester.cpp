//
// Created by silasm on 30.07.23.
//
#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

int main(int argc, char **argv){
    if(argc != 3){
        std::cerr << "Usage: groundtruth.json capiresult.json" << std::endl;
        return -1;
    }

    std::cout << "Running test for " << argv[1] << "==" << argv[2] << std::endl;

    json groundTruth;
    {
        std::ifstream file(argv[1]);
        groundTruth = json::parse(file);
    }

    json testItem;
    {
        std::ifstream file(argv[2]);
        testItem = json::parse(file);
    }

    if(groundTruth == testItem){
        std::cout << "Test success" << std::endl;
        return 0;
    }
    else {
        std::cerr << "Test failure" << std::endl;

        return 1;
    }


}
