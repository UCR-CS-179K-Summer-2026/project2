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
        null
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
    void indexStructure();
    const std::vector<TypeStruct>& getTypeIndex() const;
    Type detectValue();  
    void constructTree();
    const Node& getRoot() const;
    const std::vector<char>& getJsonData() const;
    void printTree() const;
    uint32_t findOddBackSlash(uint32_t B, bool prevBackSlash);
    uint32_t findString(uint32_t Q);
    bool backSlashEnd(uint32_t val);


private:

    std::vector<char> jsonData;
    bool inString = false;
    bool isBackSlashOdd = false;
    int stringStart = 0;
    int backSlashCount = 0;
    std::vector<TypeStruct> typeIndex;

    Node root;
    std::deque<Node*> nodes;
    std::string key;
    bool containsKey = false;

    void printNode(const Node& node, int depth) const;

    // Based on the Lookup tables used in the vectorized classification technique from
    // Langdale and Lemire, Sec. 3.1.2 / Table 1.
    uint8_t hTable[16] = {16,0,160,64,0,3,0,12,0,0,0,0,0,0,0,0};
    uint8_t lTable[16] = {32,0,0,0,0,0,0,0,0,16,80,5,128,26,0,0};

};  


#endif