#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <map>
#include <deque>
#include <list>

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
        // i am changing deque to list because deque is not working for me, it is giving me segmentation fault when I try to access the elements of the deque, so I am changing it to list and see if it works
        //std::deque is NOT guaranteed by the C++ standard to support an incomplete/self-referential element type (only vector/list/forward_list are)
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

    void printNode(const Node& node, int depth) const;


    std::vector<char> LBracket = {'{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{','{'};
    std::vector<char> RBracket = {'}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}','}'};
    std::vector<char> LBrace = {'[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','[','['};
    std::vector<char> RBrace = {']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']',']'};
    std::vector<char> Colon = {':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':'};
    std::vector<char> Comma = {',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',',','};
    std::vector<char> Quote = {'"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"','"'};
    std::vector<char> backSlash = {'\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\','\\'};

    std::vector<char> space = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
    std::vector<char> newline = {'\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n','\n'};
    std::vector<char> carriage = {'\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r','\r'};
    std::vector<char> tab = {'\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t'};
    std::vector<char> zero = {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};
};  


#endif