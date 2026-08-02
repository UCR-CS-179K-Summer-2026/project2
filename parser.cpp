#include "parser.h"

#include <iostream>
#include <fstream>


bool parser::loadFile(const std::string& s) {
    std::ifstream file(s, std::ios::binary);
    if(!file.is_open()) {
        std::cout << "File did not open. \n" << std::endl;
        return false; 
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    jsonData.resize(size);
    file.read(jsonData.data(), size);
    if(!file) {
        std::cout << "Incomplete/Failed Read \n" << std::endl;
        return false;
    }

    return true;
}


parser::quote parser::detectString(char c) {
    if(c == '"' &&  quoteStart == false) {
        quoteStart = true;
        return quote::start;
    }
    else {
        if(c == '\\') {
            backSlashCounter++;
            return quote::none;
        }
        else if(c == '"') {
            if(backSlashCounter % 2 == 0) {
                backSlashCounter = 0;
                quoteStart = false;
                return quote::end;
            }
            else {
                backSlashCounter = 0;
                return quote::none;
            }
        }
        else {
            return quote::none;
        }
    }
}


