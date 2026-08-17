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

uint32_t parser::findOddBackSlash(uint32_t B) {
    uint32_t E = 0x55555555;
    uint32_t O = 0xAAAAAAAA;

    uint32_t S = B & ~(B << 1);
    uint32_t ES = S & E;
    uint32_t EC = B + ES;
    uint32_t ECE = EC & ~B;
    uint32_t OD1 = ECE & ~E;

    uint32_t OS = S & O;
    uint32_t OC = B + OS;
    uint32_t OCE = OC & ~B;
    uint32_t OD2 = OCE & E;

    uint32_t OD = OD1 | OD2;

    return OD;
}  

uint32_t parser::findString(uint32_t Q) {
    uint32_t S0 = Q ^ (Q << 1);
    uint32_t S1 = S0 ^ (S0 << 2);
    uint32_t S2 = S1 ^ (S1 << 4);
    uint32_t S3 = S2 ^ (S2 << 8);
    uint32_t S4 = S3 ^ (S3 << 16);

    
    return S4;
}

void parser::indexStructure() {

    __m256i quote = _mm256_set1_epi8('"');
    __m256i BSlash = _mm256_set1_epi8('\\');
    __m256i space = _mm256_set1_epi8(' ');

    __m128i hTables = _mm_setr_epi8(hTable[0], hTable[1], hTable[2], hTable[3], hTable[4], hTable[5], hTable[6], hTable[7], hTable[8], hTable[9], hTable[10], hTable[11], hTable[12], hTable[13], hTable[14], hTable[15]);
    __m256i dupHTable = _mm256_broadcastsi128_si256(hTables);
    __m128i lTables = _mm_setr_epi8(lTable[0], lTable[1], lTable[2], lTable[3], lTable[4], lTable[5], lTable[6], lTable[7], lTable[8], lTable[9], lTable[10], lTable[11], lTable[12], lTable[13], lTable[14], lTable[15]);
    __m256i dupLTable = _mm256_broadcastsi128_si256(lTables);

    __m256i lowMask = _mm256_set1_epi8(0x0F);
    __m256i highMask = _mm256_set1_epi8(0x0F);


    for(size_t i  = 0; i < jsonData.size(); i+=32) {

        __m256i data = _mm256_loadu_si256 (reinterpret_cast<const __m256i*>(jsonData.data() + i));

        __m256i LN = _mm256_and_si256(data, lowMask);
        __m256i HN = _mm256_and_si256(_mm256_srli_epi16(data, 4), highMask);

        __m256i LOW = _mm256_shuffle_epi8(dupLTable, LN);
        __m256i HIGH = _mm256_shuffle_epi8(dupHTable, HN);

        __m256i result = _mm256_and_si256(LOW, HIGH);
        
        __m256i ZM = _mm256_setzero_si256();
        __m256i ZERO = _mm256_cmpeq_epi8(result, ZM);
        uint32_t SV = ~static_cast<uint32_t>(_mm256_movemask_epi8(ZERO));

        __m256i compareQuote = _mm256_cmpeq_epi8(data, quote);
        __m256i compareBackSlash = _mm256_cmpeq_epi8(data, BSlash);
        uint32_t resultQ = static_cast<uint32_t>(_mm256_movemask_epi8(compareQuote));
        uint32_t resultBac = static_cast<uint32_t>(_mm256_movemask_epi8(compareBackSlash));

        uint32_t oddNumberBSlash = findOddBackSlash(resultBac);
        uint32_t Q = resultQ & ~oddNumberBSlash;

        uint32_t stringM = findString(Q);
        SV &= ~stringM;

        uint32_t collapseResult = SV;
        //while(collapseResult != 0) {

        //}
    } 
}



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