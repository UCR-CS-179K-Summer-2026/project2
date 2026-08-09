#include "parser.h"

#include <iostream>
#include <fstream>
#include <immintrin.h>



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

/*
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
            backSlashCounter = 0;
            return Type::none;
        }
    }
}
*/

parser::Type parser::detectType(char c) {
    
    if((c >= '0' && c <= '9') || c == '-') {
        return Type::number;
    }
    else if(c == 't' || c == 'T') {
        return Type::boolean;
    }
    else if(c == 'f' || c == 'F') {
        return Type::boolean;
    }
    else {
        return Type::null;
    }


}

void parser::indexStructure() {
    for(size_t i  = 0; i < jsonData.size(); i+=32) {
        __m256i data = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(jsonData.data() + i));

        __m256i LCB = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(LBracket.data()));
        __m256i RCB = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(RBracket.data()));
        __m256i LSB= _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(LBrace.data()));
        __m256i RSB = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(RBrace.data()));
        __m256i QUO = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(Quote.data()));
        __m256i COL = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(Colon.data()));
        __m256i COM = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(Comma.data()));
        __m256i BAC = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(backSlash.data()));
        __m256i NUM = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(zero.data()));

        __m256i Space = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(space.data()));
        __m256i NewLine = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(newline.data()));
        __m256i Carriage = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(carriage.data()));
        __m256i Tab = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(tab.data()));

        __m256i compareLCB = _mm256_cmpeq_epi8 (data, LCB); //{
        __m256i compareRCB = _mm256_cmpeq_epi8 (data, RCB); //}
        __m256i compareLSB = _mm256_cmpeq_epi8 (data, LSB); //[
        __m256i compareRSB = _mm256_cmpeq_epi8 (data, RSB); //]
        __m256i compareQuote = _mm256_cmpeq_epi8 (data, QUO); //"
        __m256i compareColon = _mm256_cmpeq_epi8 (data, COL); //:
        __m256i compareComma = _mm256_cmpeq_epi8 (data, COM); //,
        __m256i compareBackSlash = _mm256_cmpeq_epi8 (data, BAC); //\

        __m256i compareSpace = _mm256_cmpeq_epi8 (data, Space); // 
        __m256i compareNewline = _mm256_cmpeq_epi8 (data, NewLine); //\n
        __m256i compareCarriage = _mm256_cmpeq_epi8 (data, Carriage); //\r
        __m256i compareTab = _mm256_cmpeq_epi8 (data, Tab); //\t

        __m256i greaterThanZero = _mm256_cmpgt_epi8(data, NUM);
        __m256i greaterThanNine = _mm256_cmpgt_epi8(data, COL);
        
        int resultLCB = _mm256_movemask_epi8(compareLCB); 
        int resultRCB = _mm256_movemask_epi8(compareRCB); 
        int resultLSB = _mm256_movemask_epi8(compareLSB); 
        int resultRSB = _mm256_movemask_epi8(compareRSB); 
        int resultQ = _mm256_movemask_epi8(compareQuote); 
        int resultCol = _mm256_movemask_epi8(compareColon);  
        int resultCom = _mm256_movemask_epi8(compareComma); 
        int resultBac = _mm256_movemask_epi8(compareBackSlash); 
       
        int resultSpace = _mm256_movemask_epi8(compareSpace); 
        int resultNewline = _mm256_movemask_epi8(compareNewline); 
        int resultCarriage = _mm256_movemask_epi8(compareCarriage); 
        int resultTab = _mm256_movemask_epi8(compareTab); 

        int resultZero = _mm256_movemask_epi8(greaterThanZero);
        int resultNine = _mm256_movemask_epi8(greaterThanNine);


        for(size_t j = 0; j < 32; j++) {

            if(resultLCB & (1 << j) && !inString && !inValue) {
                typeIndex.push_back({Type::objectStart, i + j});
            }
            else if(resultRCB & (1 << j) && !inString && !inValue) {
                typeIndex.push_back({Type::objectEnd, i + j});
            }
            else if(resultLSB & (1 << j) && !inString && !inValue) {
                typeIndex.push_back({Type::arrayStart, i + j});
            }
            else if(resultRSB & (1 << j) && !inString && !inValue) {
                typeIndex.push_back({Type::arrayEnd, i + j});
            }
            else if((resultQ & (1 << j)) && !inValue) {
                if(inString == false) {
                    inString = true;
                    typeIndex.push_back({Type::quoteStart, i + j});
                }
                else {
                    if(backSlashCounter % 2 == 0) {
                        typeIndex.back().ePosition = i + j;
                        typeIndex.back().type = Type::string;
                        inString = false;
                    } 
                    else {
                        backSlashCounter = 0;
                    } 
                }
            }
            else if(resultCol & (1 << j) && !inString && !inValue) {
                typeIndex.push_back({Type::colon, i + j});
            }
            else if(resultCom & (1 << j) && !inString && !inValue) {
                typeIndex.push_back({Type::comma, i + j});
            }
            else if((resultBac & (1 << j)) && !inValue) {
                backSlashCounter++;
            }
            else if((resultZero & (1 << j)) && (resultNine & (1 << j)) && !inString) {
                inValue = true;
                typeIndex.push_back({Type::number, i + j});
            }
            else if(((resultSpace & (1 << j)) || (resultNewline & (1 << j)) || (resultCarriage & (1 << j)) || (resultTab & (1 << j))) && inValue) {
                inValue = false;
                typeIndex.back().ePosition = i + j - 1;
            }
            else {
                backSlashCounter = 0;
            }
        }
    } 
}

/*
void parser::indexStructure() {
    for(size_t i = 0; i < jsonData.size(); i++) {
        char c = jsonData.at(i);

        if(inString) {
            Type t = detectString(jsonData.at(i));
            if(t != Type::none) {
                typeIndex.back().ePosition = i;
                typeIndex.back().type = Type::string;
            }
            continue;
        }

        if(inValue) {
            bool structure;
            structure = (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '"' || c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',');
            if(structure) {
                typeIndex.back().ePosition = i - 1;
                inValue = false;
            }
        }

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
        else if(c == ' ' || c == '\n' || c == '\r' || c == '\t') {

        }
        else if(!inValue) {
            typeIndex.push_back({detectType(c), i});
            inValue = true;
        }
        
    }
    
}

*/

const std::vector<parser::TypeStruct>& parser::getTypeIndex() const {
    return typeIndex;
}


void parser::constructTree() {
    for(size_t i = 0; i < typeIndex.size(); i++) {
        if(typeIndex.at(i).type == Type::objectStart) {
            Node newNode;
            newNode.nodeType = NodeType::object;
            if(nodes.empty()) {
                root = newNode;
                nodes.push_back(&root);
            }
            else if(nodes.back()->nodeType == NodeType::object) {
                nodes.back()->objectChildNode[key] = newNode;
                nodes.push_back(&(nodes.back()->objectChildNode[key]));
            }
            else if(nodes.back()->nodeType == NodeType::array) {
                nodes.back()->arrayChildNode.push_back(newNode);
                nodes.push_back(&(nodes.back()->arrayChildNode.back()));
                
            }
        }
        else if(typeIndex.at(i).type == Type::arrayStart) {
            Node newNode;
            newNode.nodeType = NodeType::array;
            if(nodes.empty()) {
                root = newNode;
                nodes.push_back(&root);
            }
            else if(nodes.back()->nodeType == NodeType::object) {
                nodes.back()->objectChildNode[key] = newNode;
                nodes.push_back(&(nodes.back()->objectChildNode[key]));
            }
            else if(nodes.back()->nodeType == NodeType::array) {
                nodes.back()->arrayChildNode.push_back(newNode);
                nodes.push_back(&(nodes.back()->arrayChildNode.back()));
            }
        }
        else if(typeIndex.at(i).type == Type::string) {
            // NEW: guard against a string being the very last token (for truncated file)
            bool nextIsColon = (i + 1 < typeIndex.size()) &&
                                (typeIndex.at(i + 1).type == Type::colon);

            if(nextIsColon) {
                std::string value(jsonData.data() + typeIndex.at(i).position + 1, jsonData.data() + typeIndex.at(i).ePosition);
                key = value;
                containsKey = true;
            }
            else {
                Node newNode;
                newNode.nodeType = NodeType::string;
                newNode.position = typeIndex.at(i).position;
                newNode.ePosition = typeIndex.at(i).ePosition;

                // NEW: guard against nodes being empty (bare top-level scalar, malformed input)
                if(!nodes.empty() && nodes.back()->nodeType == NodeType::object) {
                    nodes.back()->objectChildNode[key] = newNode;
                    containsKey = false;
                }
                else if(!nodes.empty() && nodes.back()->nodeType == NodeType::array) {
                    nodes.back()->arrayChildNode.push_back(newNode);
                }
            
            }  
        }
        else if(typeIndex.at(i).type == Type::number) {
            Node newNode;
            newNode.nodeType = NodeType::number;
            newNode.position = typeIndex.at(i).position;
            newNode.ePosition = typeIndex.at(i).ePosition;

            if(!nodes.empty() && nodes.back()->nodeType == NodeType::object) {
                nodes.back()->objectChildNode[key] = newNode;
                containsKey = false;
            }
            else if(!nodes.empty() && nodes.back()->nodeType == NodeType::array) {
                nodes.back()->arrayChildNode.push_back(newNode);
            }

            
        }
        else if(typeIndex.at(i).type == Type::boolean) {
            Node newNode;
            newNode.nodeType = NodeType::boolean;
            newNode.position = typeIndex.at(i).position;
            newNode.ePosition = typeIndex.at(i).ePosition;

            if(!nodes.empty() && nodes.back()->nodeType == NodeType::object) {
                nodes.back()->objectChildNode[key] = newNode;
                containsKey = false;
            }
            else if(!nodes.empty() && nodes.back()->nodeType == NodeType::array) {
                nodes.back()->arrayChildNode.push_back(newNode);
            }
            
        }
        else if(typeIndex.at(i).type == Type::null) {
            Node newNode;
            newNode.nodeType = NodeType::null;
            newNode.position = typeIndex.at(i).position;
            newNode.ePosition = typeIndex.at(i).ePosition;

            if(!nodes.empty() && nodes.back()->nodeType == NodeType::object) {
                nodes.back()->objectChildNode[key] = newNode;
                containsKey = false;
            }
            else if(!nodes.empty() && nodes.back()->nodeType == NodeType::array) {
                nodes.back()->arrayChildNode.push_back(newNode);
            }
            
        }
        else if(typeIndex.at(i).type == Type::objectEnd || typeIndex.at(i).type == Type::arrayEnd) {
            if(!nodes.empty()) {  // NEW: guard against popping an already-empty stack
                nodes.pop_back();
            }
        }
    }
}

// NEW: accessors
const parser::Node& parser::getRoot() const {
    return root;
}

const std::vector<char>& parser::getJsonData() const {
    return jsonData;
}


// NEW: recursive print, so the tree can actually be verified by eye. Since Leaf nodes only store position/ePosition , so this pulls the raw substring back out of json data to display the actual value.
void parser::printNode(const Node& node, int depth) const {
    std::string indent(depth * 2, ' ');

    switch(node.nodeType) {
        case NodeType::object: {
            std::cout << indent << "{" << std::endl;
            for(const auto& [childKey, childNode] : node.objectChildNode) {
                std::cout << indent << "  \"" << childKey << "\": ";
                // if child is a container, newline before recursing; otherwise print inline
                if(childNode.nodeType == NodeType::object || childNode.nodeType == NodeType::array) {
                    std::cout << std::endl;
                    printNode(childNode, depth + 1);
                }
                else {
                    printNode(childNode, 0); // inline leaf, no extra indent needed
                }
            }
            std::cout << indent << "}" << std::endl;
            break;
        }
        case NodeType::array: {
            std::cout << indent << "[" << std::endl;
            for(const auto& childNode : node.arrayChildNode) {
                if(childNode.nodeType == NodeType::object || childNode.nodeType == NodeType::array) {
                    printNode(childNode, depth + 1);
                }
                else {
                    std::cout << indent << "  ";
                    printNode(childNode, 0);
                }
            }
            std::cout << indent << "]" << std::endl;
            break;
        }
        case NodeType::string: {
            // position/ePosition point at the opening/closing '"' themselves, so will exclude both quote chars here, then add our own quotes for display.
            std::string value(jsonData.data() + node.position + 1, jsonData.data() + node.ePosition);
            std::cout << "\"" << value << "\"" << std::endl;
            break;
        }
        case NodeType::number: {
            std::string value(jsonData.data() + node.position, jsonData.data() + node.ePosition + 1);
            std::cout << value << std::endl;
            break;
        }
        case NodeType::boolean: {
            std::string value(jsonData.data() + node.position, jsonData.data() + node.ePosition + 1);
            std::cout << value << std::endl;
            break;
        }
        case NodeType::null: {
            std::cout << "null" << std::endl;
            break;
        }
    }
}

void parser::printTree() const {
    printNode(root, 0);
}