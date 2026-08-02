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

    return 0;
}

