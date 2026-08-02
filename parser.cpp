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


parser::Type parser::detectString(char c) {
    if(c == '"' &&  inString == false) {
        inString = true;
        return Type::quoteStart;
    }
    else {
        if(c == '\\') {
            backSlashCounter++;
            return Type::none;
        }
        else if(c == '"') {
            if(backSlashCounter % 2 == 0) {
                backSlashCounter = 0;
                inString = false;
                return Type::quoteEnd;
            }
            else {
                backSlashCounter = 0;
                return Type::none;
            }
        }
        else {
            return Type::none;
        }
    }
}


void parser::indexStructure() {
    for(size_t i = 0; i < jsonData.size(); i++) {
        if(inString) {
            Type t = detectString(jsonData.at(i));
            if(t != Type::none) {
                typeIndex.push_back({t, i});
            }
        }
        else { 
            if(jsonData.at(i) == '"') {
                typeIndex.push_back({detectString(jsonData.at(i)), i});
            }
            else if(jsonData.at(i) == '[') {
                typeIndex.push_back({Type::arrayStart, i});
            }
            else if(jsonData.at(i) == ']') {
                typeIndex.push_back({Type::arrayEnd, i});
            }
            else if(jsonData.at(i) == '{') {
                typeIndex.push_back({Type::objectStart, i});
            }
            else if(jsonData.at(i) == '}') {
                typeIndex.push_back({Type::objectEnd, i});
            }
            else if(jsonData.at(i) == ',') {
                typeIndex.push_back({Type::comma, i});
            }
            else if(jsonData.at(i) == ':') {
                typeIndex.push_back({Type::colon, i});
            }
        }
    }
}



