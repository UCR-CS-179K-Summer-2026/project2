//so this is Tasnim's part and is not fully implemented yet, but this is kind of structure I am expecting her to hand me off and will make the adjustments 
//accordingly if her structure is different from mine. this is just a prototype

#pragma once
 
#include <string>   // for std::string (holding field names like "employees")
#include <vector>   // for std::vector (holding a list of steps)
 
enum class StepKind { Field, Wildcard }; //field = go into this specific key". wildcard = "expand every element of the current array".
//used enum class since here it stops StepKind::Field from being accidentally compared to some unrelated integer.
 
struct QueryStep {
    StepKind kind; //which of the two instruction types this step is
    std::string fieldName; // only used when kind == StepKind::Field, for wildcard it left empty or unused /
};
 
using QueryAST = std::vector<QueryStep>;