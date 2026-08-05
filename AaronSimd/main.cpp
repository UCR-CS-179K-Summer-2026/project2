#include "parser.h"
#include <iostream>
#include <fstream>
#include <vector>


int main() {
    parser p;
    if(!p.loadFile("Employee.json")) {
        return 1;
    }
    p.indexStructure();
    p.constructTree();

    // NEW: actually show the tree so it can be verified
    std::cout << "--- Parsed tree ---" << std::endl;
    p.printTree();

    return 0;
}