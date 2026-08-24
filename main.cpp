#include "AaronJsonParser/parser.h"
#include "TasnimQueryParser/QueryParser.h"
#include "query_executor.h"
#include <iostream>
#include <string>

using namespace std;


void showHelp() {
	while (true) {
		string helpChoice;

		cout << "\n========================================\n";
		cout << "              QUERY HELP\n";
		cout << "========================================\n";

		cout << "What do you need help with?\n\n";

		cout << "  1: Query Types\n";
		cout << "  2: Supported Operators\n";
		cout << "  3: String Values\n";
		cout << "  4: Result Values\n";
		cout << "  5: Commands\n";
		cout << "  6: Back\n";

		cout << "\nEnter choice: ";
		getline(cin, helpChoice);

		if (helpChoice == "1") {
			cout << "\nQUERY TYPES\n";
			cout << "----------------------------------------\n";

			cout << "Dot path queries:\n";
			cout << "  Syntax:  <key>.<key>.<field>\n";
			cout << "  Example: store.products.name\n";

			cout << "\nArray index (one element):\n";
			cout << "  Syntax:  <key>.<array>[<index>].<field>\n";
			cout << "  Example: store.products[0].name\n";

			cout << "\nWildcard queries (all elements):\n";
			cout << "  Syntax:  <key>.<array>[*].<field>\n";
			cout << "  Example: store.products[*].name\n";

			cout << "\nFilter queries:\n";
			cout << "  Syntax:  GET <field> FROM <path> WHERE <field> <operator> <value>\n";
			cout << "  Example: GET name FROM store.products WHERE price > 300\n";

			cout << "\nMultiple conditions:\n";
			cout << "  Syntax:  GET <field> FROM <path> WHERE <condition> AND <condition>\n";
			cout << "  Example: GET name FROM store.products WHERE price > 300 AND inStock = true\n";
		}

		else if (helpChoice == "2") {
			cout << "\nSUPPORTED OPERATORS FOR FILTER QUERIES\n";
			cout << "----------------------------------------\n";
			cout << "  =   Equal\n";
			cout << "  !=  Not equal\n";
			cout << "  <   Less than\n";
			cout << "  <=  Less than or equal to\n";
			cout << "  >   Greater than\n";
			cout << "  >=  Greater than or equal to\n";
		}

		else if (helpChoice == "3") {
			cout << "\nSTRING VALUES\n";
			cout << "----------------------------------------\n";
			cout << "Strings containing spaces should use quotes.\n\n";
			cout << "Examples:\n";
			cout << "  name = 'John Doe'\n";
			cout << "  name = \"John Doe\"\n";
		}

		else if (helpChoice == "4") {
			cout << "\nRESULT VALUES\n";
			cout << "----------------------------------------\n";
			cout << "  null: Actual JSON null value\n";
			cout << "  DNE:  Requested path does not exist\n";
		}

		else if (helpChoice == "5") {
			cout << "\nCOMMANDS\n";
			cout << "----------------------------------------\n";
			cout << "  HELP  Open the help menu\n";
			cout << "  QUIT  Exit the program\n";
			cout << "\nMENU  After running a query, the options are to:\n";
			cout << "  Enter another query\n";
			cout << "  Load a different JSON file\n";
			cout << "  Exit\n";
		}

		else if (helpChoice == "6") {
            break;
        }

		else {
			cout << "\nInvalid help option. Please enter numbers from 1 - 6 to corresponding choice.\n";
		}
	}
}


int main() {
    QueryParser qp;


	cout << "========================================\n";
	cout << "                                        \n";
    cout << "          JSON QUERY ENGINE\n";
	cout << "                                        \n";
    cout << "========================================\n\n";


    while (true) {
		parser p;
		string filename;

		while (true) {
			cout << "Enter JSON file name (or type QUIT / HELP): ";
			getline(cin, filename);

			if (filename == "QUIT") {
				cout << "Exiting.\n";
				return 0;
			}

			if (filename == "HELP") {
				showHelp();
				continue;
			}

			if (p.loadFile(filename)) {
				cout << "File loaded successfully.\n";
				break;
			}

			cout << "Could not load file. Please try again.\n\n";
		}
		

		p.indexStructure();
		p.constructTree();

		//query loop
		while (true) {
			string queryText;

			cout << "\nEnter a query (or type HELP / QUIT):\n> ";
			getline(cin, queryText);

			if (queryText == "QUIT") {
				cout << "Exiting program.\n";
				return 0;
			}

			//opening help menu
			if (queryText == "HELP") {
				showHelp();
				continue;
			}


			try {
				Query query = qp.parse(queryText);

				auto results = executeQuery(
					p.getRoot(),
					query,
					p.getJsonData()
				);

				cout << "\n========================================\n";
				cout << "                RESULTS\n";
				cout << "\n========================================\n";

				cout << "["

				for (size_t i = 0; i < results.size(); ++i) {
					cout << nodeToString(results[i], p.getJsonData());

					if (i + 1 < results.size()) {
						cout << ", ";
					}
				}

				cout << "]\n\n";
				cout << results.size() << " result(s) found.\n";
			}
			catch (const exception& e) {
				cout << "\nERROR: " << e.what() << '\n';
			}

			string menuChoice;

			cout << "\nWhat would you like to do?\n";
			cout << "  1: Enter another query\n";
			cout << "  2: Load a different JSON file\n";
			cout << "  3: Help\n";
			cout << "  4: Quit\n";

			cout << "\nEnter choice (1-4): ";
			getline(cin, menuChoice);

			if (menuChoice == "1") {
				continue;
			}

			else if (menuChoice == "2") {
				break;
			}

			else if (menuChoice == "3") {
				showHelp();
				continue;
			}

			else if (menuChoice == "4") {
				cout << "Exiting program.\n";
				return 0;
			}

			else {
				cout << "Invalid option. Please enter a number from 1 to 4.\n";
			}
		
		}

	}	
	return 0;

}