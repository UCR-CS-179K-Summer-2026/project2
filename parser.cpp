#include <iostream>
#include <fstream>
#include <vector>


int main() {

    std::vector<char> jsonData;
    std::ifstream file("Employee.json", std::ios::binary);
    
    
    if(!file.is_open()) {
        std::cout << "File did not open. \n";
        return 1;
    }

    file.seekg(0, std::ifstream::end);
    auto size = file.tellg();
    file.seekg(0, std::ifstream::beg);

    jsonData.resize(size);
    file.read(jsonData.data(), size);
    if (!file) {
        std::cout << "Incomplete/Failed Read \n";
        return 1;
    }

    bool inString = false;
    int counter = 0;
    
    for(int i = 0; i < jsonData.size(); i++) {
        
        if(inString == false) {
            if(jsonData.at(i) == '"') {
                inString = true;
            }
        }
        else {
            if(jsonData.at(i) == '\\') {
                counter++;
            }
            else if(jsonData.at(i) == '"') {
                if(counter % 2 == 0) {
                    
                    inString = false;
                    counter = 0;
                }
                else {
                    
                    counter = 0;
                }
            }
            else {
                counter = 0;
            }
        }
    }
}

