#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <map>
#include <deque>

class parser {
public:
    enum class Type {
        arrayStart,
        arrayEnd,
        quoteStart,
        quoteEnd,
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
        std::deque<Node> arrayChildNode;

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

private:

    std::vector<char> jsonData;
    int backSlashCounter = 0;
    bool inString = false;
    bool inValue = false;
    std::vector<TypeStruct> typeIndex;
    

    Node root;
    std::deque<Node*> nodes;
    std::string key;
    bool containsKey = false;
};


#endif