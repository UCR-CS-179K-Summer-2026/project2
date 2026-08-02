#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>

class parser {
public:
    bool loadFile(const std::string& s);
    Type detectString(char c);
    void indexStructure();
    void getTypeIndex;
private:

    std::vector<char> jsonData;
    int backSlashCounter = 0;
    bool inString = false;

    enum class Type {
        arrayStart,
        arrayEnd,
        quoteStart,
        quoteEnd,
        objectStart,
        objectEnd,
        comma,
        colon,
        none
    };

    struct TypeStruct {
        Type type;
        size_t position;
    };

    std::vector<TypeStruct> typeIndex;

};


#endif