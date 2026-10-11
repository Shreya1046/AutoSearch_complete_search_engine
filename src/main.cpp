#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include "Trie.h"
#include "TopK.h"

using namespace std;

int main() {
    Trie trie;

    // STEP 1: Load the dataset
    ifstream file("data/queries.txt");

    if (!file.is_open()) {
        cerr << "Error: Could not open data/queries.txt\n";
        cerr << "Run the program from the project root folder.\n";
        return 1;
    }

    string line;
    int loadedQueries = 0;

    while (getline(file, line)) {
        size_t comma = line.find(',');

        if (comma == string::npos) {
            continue;
        }

        string query = line.substr(0, comma);
        string frequencyText = line.substr(comma + 1);

        try {
            size_t parsedCharacters = 0;

            int frequency = stoi(
                frequencyText,
                &parsedCharacters
            );

            if (parsedCharacters != frequencyText.size() ||
                frequency < 0 ||
                query.empty()) {
                continue;
            }

            trie.insert(query, frequency);
            loadedQueries++;
        }
        catch (const exception&) {
            continue;
        }
    }

    file.close();

    // STEP 2: Display the application header
    cout << "====================================\n";
    cout << "      AUTOCOMPLETE SEARCH ENGINE    \n";
    cout << "====================================\n";

    cout << "Dataset entries loaded: "
         << loadedQueries << "\n";

    // STEP 3: Accept the user's prefix
    string prefix;

    cout << "\nEnter search prefix: ";
    getline(cin, prefix);

    // STEP 4: Retrieve matching candidates
    vector<pair<string, int>> suggestions =
        trie.autocomplete(prefix);

    // STEP 5: Select the Top-K candidates using our custom heap
    const int TOP_K = 5;

    TopK topK(TOP_K);

    for (const auto& suggestion : suggestions) {
        topK.add(suggestion);
    }

    vector<pair<string, int>> results = topK.getResults();

    // STEP 6: Display the final results
    cout << "\nTop suggestions:\n";

    if (results.empty()) {
        cout << "No suggestions found.\n";
    } else {
        for (size_t i = 0; i < results.size(); i++) {
            cout << i + 1 << ". "
                 << results[i].first
                 << " (Frequency: "
                 << results[i].second
                 << ")\n";
        }
    }

    cout << "\nTotal matching suggestions: "
         << suggestions.size() << '\n';

    return 0;
}