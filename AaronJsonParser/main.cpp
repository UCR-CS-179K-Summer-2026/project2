#include "parser.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

int main() {
    parser p;
    if(!p.loadFile("data_1gb.json")) {
        return 1;
    }
    auto start = std::chrono::steady_clock::now();
    p.indexStructure();
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(end - start);
    std::cout << "indexStructure function took " << duration.count() << " seconds" << std::endl;



    //p.constructTree();
    //std::cout << "--- Parsed tree ---" << std::endl;
    //p.printTree();

    return 0;
}