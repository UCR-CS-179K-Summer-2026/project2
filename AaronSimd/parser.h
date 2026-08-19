#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <map>
#include <deque>
#include <list>
#include <cstdint>

class parser {
public:
    enum class Type {
        arrayStart,
        arrayEnd,
        objectStart,
        objectEnd,
        comma,
        colon,
        string,
        number,
        boolean,
        null,
        none
    };
    enum class NodeType {
        object,
        array,
        string,
        number,
        boolean,
        null
    };


    struct Node {
        NodeType nodeType;
        std::map<std::string, Node> objectChildNode;
        std::list<Node> arrayChildNode;

        size_t position;
        size_t ePosition;
    };

    struct TypeStruct {
        Type type;
        size_t position;
        size_t ePosition;
    };

    bool loadFile(const std::string& s);
    Type detectString(char c);
    Type detectType(char c);
    void indexStructure();
    const std::vector<TypeStruct>& getTypeIndex() const;
    Type detectValue();  
    void constructTree();
    const Node& getRoot() const;
    const std::vector<char>& getJsonData() const;
    void printTree() const;
    uint32_t findOddBackSlash(uint32_t B);
    uint32_t findString(uint32_t Q);


private:

    std::vector<char> jsonData;
    int backSlashCounter = 0;
    bool inString = false;
    int stringStart = 0;
    bool inValue = false;
    std::vector<TypeStruct> typeIndex;

    Node root;
    std::deque<Node*> nodes;
    std::string key;
    bool containsKey = false;

    void printNode(const Node& node, int depth) const;
    uint8_t hTable[16] = {64,0,32,16,0,12,0,3,0,0,0,0,0,0,0,0};
    uint8_t lTable[16] = {0,0,0,0,0,0,0,0,0,64,80,5,32,74,0,0};

};  


#endif