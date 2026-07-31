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

}