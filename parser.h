#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>

Class parser {
public:
    bool openFile(const std::string& s);
    quote detectString(char c);

private:

    std::vector<char> jsonData;
    int backSlashCounter = 0;
    bool quoteStart = 0;
    enum class quote {
        none,
        start,
        end
    };

};


#endif