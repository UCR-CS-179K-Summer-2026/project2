#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>

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

private:

    std::vector<char> jsonData;
    int backSlashCounter = 0;
    bool inString = false;
    bool inValue = false;
    std::vector<TypeStruct> typeIndex;

};


#endif