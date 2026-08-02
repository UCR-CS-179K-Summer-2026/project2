//now this is Aaron's part but this is my assumption of something we agreed upon, although his implementation might be different so later on 
// so I will adjust to his version when he finishes his part.

//this is the structure of JSON parser we all discussed but original version of AAron might be bit different 
#pragma once
 
#include <string>   // for std::string (holding text like "John Doe")
#include <vector>   // for std::vector (holding array elements)
#include <map>      // for std::map (holding object key->value pairs)
#include <memory>   // for std::shared_ptr (holding pointers to child nodes)
 
struct JsonValue;
using JsonValuePtr = std::shared_ptr<JsonValue>;
 
enum class JsonType { Null, Bool, Number, String, Array, Object };
 
struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    std::string strVal;
    std::vector<JsonValuePtr> arrVal;
    std::map<std::string, JsonValuePtr> objVal;
 
    std::string toString() const {
        switch (type) {
            case JsonType::Null:   return "null";
            case JsonType::Bool:   return boolVal ? "true" : "false";
            case JsonType::Number: {
                if (numVal == (long long)numVal)
                    return std::to_string((long long)numVal);
                return std::to_string(numVal);
            }
            case JsonType::String: return "\"" + strVal + "\""; //so it prints like "John Doe" instead of just John Doe
            default: return "<complex>"; //Rather than trying to print an entire nested array/object as a one-line string (which gets complicated fast
                                        //nested brackets, commas, indentation...), we just punt and print a placeholder "<complex>"
        }
    }
};