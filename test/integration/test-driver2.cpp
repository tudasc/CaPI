//
// Created by silasm on 30.07.23.
//
#include "json.hpp"

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
   
   for(auto& el : testItem.items()){
       for(auto& gt : groundTruth.items()){
           for(auto& itvalue : el.value()){
               //std::cout << gt.value() << std::endl;
              // for(auto& gti : gt.value().contains){
                /*if(gt.value().contains(it))
                    std::cout << "True" << std::endl;
                else std::cout << "False" << std::endl;              // }
                */
               bool found = false;
               for(auto& gtvalue : gt.value()){
                   if(itvalue == gtvalue){
                    found = true;
                    break;
                   }
                   else found = false;
               }

               if(found) continue; 
               else
                 std::cout << "Test failure";
                 return 1;
           }

        }
    }

    std::cout << "Test success!" << std::endl;
    return 0;
/*
    if(groundTruth == testItem){
        std::cout << "Test success" << std::endl;
        return 0;
    }
    else {
        std::cerr << "Test failure" << std::endl;

        return 1;
    }
*/


}
