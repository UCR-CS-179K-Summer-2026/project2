#include "AaronJsonParser/parser.h"
#include "TasnimQueryParser/QueryParser.h"
#include "query_executor.h"
#include <iostream>

using namespace std;


int main() {
    parser p;
    QueryParser qp;

    string filename;

	cout << "========================================\n";
    cout << "          JSON QUERY ENGINE\n";
    cout << "========================================\n\n";


    while (true) {
        cout << "Enter JSON file name (or type QUIT): ";
        getline(cin, filename);

        if (filename == "QUIT") {
            cout << "Exiting program.\n";
            return 0;
        }

        if (p.loadFile(filename)) {
            cout << "File loaded successfully.\n";
            break;
        }

        cout << "Could not load file. Please try again.\n\n";
    }

    p.indexStructure();
    p.constructTree();

    return 0;
}