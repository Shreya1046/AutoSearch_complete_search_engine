#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

#include "Trie.h"
#include "TopK.h"

using namespace std;

const string DATA_DIR = "data/";

const string BIG_DATASET =
    DATA_DIR + "search-queries-1.3M.csv";

const string JAVA_DATASET =
    DATA_DIR + "java_queries.csv";

const string CSHARP_DATASET =
    DATA_DIR + "csharp_queries.csv";

const string COMBINED_DATASET =
    DATA_DIR + "queries.txt";

const string TECHNOLOGY_DATASET =
    DATA_DIR + "technology_queries.txt";

const string USER_SCORES_FILE =
    DATA_DIR + "user_scores.txt";

const size_t TOP_K = 5;

Trie searchTrie;


// Trim whitespace at the beginning and end.
string trim(const string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");

    if (start == string::npos)
        return "";

    size_t end = text.find_last_not_of(" \t\r\n");

    return text.substr(start, end - start + 1);
}


// Parse a number safely.
bool parseNumber(
    const string& text,
    long long& number
) {
    string value = trim(text);

    if (value.empty())
        return false;

    char* end = NULL;

    long long result = strtoll(
        value.c_str(),
        &end,
        10
    );

    if (end == value.c_str() || *end != '\0')
        return false;

    number = result;
    return true;
}


// Parse CSV rows, including quoted fields and escaped quotes.
vector<string> parseCSV(const string& line) {
    vector<string> fields;
    string field;
    bool insideQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];

        if (ch == '"') {
            if (insideQuotes &&
                i + 1 < line.size() &&
                line[i + 1] == '"') {

                field.push_back('"');
                ++i;
            } else {
                insideQuotes = !insideQuotes;
            }
        } else if (ch == ',' && !insideQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field.push_back(ch);
        }
    }

    fields.push_back(field);

    return fields;
}


// Convert a popularity rank to an initial ranking score.
// Rank 1 receives the highest score.
// This score is NOT a raw search count.
long long rankToScore(long long rank) {
    if (rank < 1)
        return 1;

    long long score = 1000000 / rank;

    return score > 0 ? score : 1;
}


void addQuery(
    const string& query,
    long long score
) {
    searchTrie.insert(
        query,
        score > 0 ? score : 1
    );
}


// Load the large query dataset.
// The observed format has one query per line.
void loadLargeDataset() {
    ifstream file(
        BIG_DATASET.c_str(),
        ios::in | ios::binary
    );

    if (!file.is_open()) {
        cout << "[WARN] Could not open "
             << BIG_DATASET << "\n";
        return;
    }

    string line;
    long long count = 0;

    while (getline(file, line)) {
        // Remove a UTF-8 BOM if present at the start.
        if (count == 0 &&
            line.size() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {

            line.erase(0, 3);
        }

        string query = trim(line);

        if (query.empty())
            continue;

        // If the line is a single quoted CSV field,
        // remove the CSV quoting.
        vector<string> fields = parseCSV(query);

        if (fields.size() == 1)
            query = fields[0];

        addQuery(query, 1);
        ++count;

        if (count % 100000 == 0) {
            cout << "Large dataset: "
                 << count << " lines processed...\n";
        }
    }

    cout << "[OK] Large dataset loaded: "
         << count << " lines.\n";
}


// Load a Search4Code CSV.
// Expected columns include QueryString and PopularityRank.
void loadSearch4Code(
    const string& path,
    const string& label
) {
    ifstream file(
        path.c_str(),
        ios::in | ios::binary
    );

    if (!file.is_open()) {
        cout << "[WARN] Could not open "
             << path << "\n";
        return;
    }

    string line;

    if (!getline(file, line))
        return;

    vector<string> header = parseCSV(line);

    int queryColumn = -1;
    int rankColumn = -1;

    for (size_t i = 0; i < header.size(); ++i) {
        string name = header[i];

        // Lowercase ASCII letters in the column name.
        for (size_t j = 0; j < name.size(); ++j) {
            if (name[j] >= 'A' && name[j] <= 'Z')
                name[j] = name[j] - 'A' + 'a';
        }

        name = trim(name);

        if (name == "querystring" ||
            name == "query" ||
            name == "searchquery") {
            queryColumn = static_cast<int>(i);
        }

        if (name == "popularityrank" ||
            name == "rank") {
            rankColumn = static_cast<int>(i);
        }
    }

    if (queryColumn == -1) {
        cout << "[WARN] No QueryString column found in "
             << path << "\n";
        return;
    }

    long long count = 0;

    while (getline(file, line)) {
        vector<string> fields = parseCSV(line);

        if (queryColumn >= static_cast<int>(fields.size()))
            continue;

        string query = fields[queryColumn];

        long long rank = 1;

        if (rankColumn >= 0 &&
            rankColumn < static_cast<int>(fields.size())) {
            parseNumber(fields[rankColumn], rank);
        }

        addQuery(query, rankToScore(rank));
        ++count;
    }

    cout << "[OK] " << label << ": "
         << count << " records loaded.\n";
}


// Load query<TAB>score or query,score files.
void loadSimpleDataset(
    const string& path,
    const string& label
) {
    ifstream file(
        path.c_str(),
        ios::in | ios::binary
    );

    if (!file.is_open()) {
        cout << "[INFO] Optional file not found: "
             << path << "\n";
        return;
    }

    string line;
    long long count = 0;

    while (getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        size_t separator = line.find('\t');

        if (separator == string::npos)
            separator = line.rfind(',');

        string query = line;
        long long score = 1;

        if (separator != string::npos) {
            query = line.substr(0, separator);

            long long parsedScore = 0;

            if (parseNumber(
                    line.substr(separator + 1),
                    parsedScore
                ) && parsedScore > 0) {

                score = parsedScore;
            }
        }

        addQuery(query, score);
        ++count;
    }

    cout << "[OK] " << label << ": "
         << count << " records loaded.\n";
}


// Restore the user's previous selections.
void loadUserScores() {
    ifstream file(
        USER_SCORES_FILE.c_str(),
        ios::in | ios::binary
    );

    if (!file.is_open())
        return;

    string line;

    while (getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        size_t separator = line.find('\t');

        if (separator == string::npos)
            continue;

        string query = line.substr(0, separator);

        long long count = 0;

        if (parseNumber(
                line.substr(separator + 1),
                count
            ) && count > 0) {

            searchTrie.applySavedSelections(
                query,
                count
            );
        }
    }

    cout << "[OK] Previous selections restored.\n";
}


// Save selection counts separately, leaving original datasets unchanged.
bool saveUserScores() {
    const vector<QueryRecord>& records =
        searchTrie.getRecords();

    string tempPath = USER_SCORES_FILE + ".tmp";

    ofstream file(
        tempPath.c_str(),
        ios::out | ios::binary
    );

    if (!file.is_open())
        return false;

    file << "# query<TAB>selection_count\n";

    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].selections > 0) {
            file << records[i].query
                 << '\t'
                 << records[i].selections
                 << '\n';
        }
    }

    file.close();

    if (!file)
        return false;

    string backupPath = USER_SCORES_FILE + ".bak";

    remove(backupPath.c_str());

    bool hadOldFile =
        (rename(
            USER_SCORES_FILE.c_str(),
            backupPath.c_str()
        ) == 0);

    if (rename(
            tempPath.c_str(),
            USER_SCORES_FILE.c_str()
        ) != 0) {

        if (hadOldFile) {
            rename(
                backupPath.c_str(),
                USER_SCORES_FILE.c_str()
            );
        }

        return false;
    }

    remove(backupPath.c_str());

    return true;
}


// Return the top five suggestions for a prefix.
vector<Suggestion> getSuggestions(
    const string& prefix
) {
    TopK topK(TOP_K);

    struct Collector {
        TopK& top;

        Collector(TopK& t) : top(t) {}

        void operator()(const QueryRecord& record) {
            top.add(
                Suggestion(
                    record.query,
                    record.score
                )
            );
        }
    };

    Collector collector(topK);

    searchTrie.forEachMatch(
        prefix,
        collector
    );

    return topK.getResults();
}


int main() {
    cout << "=====================================\n";
    cout << " C++ TECHNOLOGY AUTOCOMPLETE ENGINE\n";
    cout << "=====================================\n\n";

    cout << "Loading datasets...\n";
    cout << "The large dataset may take some time.\n\n";

    loadLargeDataset();

    loadSearch4Code(
        JAVA_DATASET,
        "Java dataset"
    );

    loadSearch4Code(
        CSHARP_DATASET,
        "C# dataset"
    );

    loadSimpleDataset(
        COMBINED_DATASET,
        "queries.txt"
    );

    loadSimpleDataset(
        TECHNOLOGY_DATASET,
        "technology queries"
    );

    loadUserScores();

    cout << "\nUnique queries loaded: "
         << searchTrie.queryCount() << "\n";

    cout << "\nCommands:\n";
    cout << "  Type a prefix to search.\n";
    cout << "  Type :add to add a query.\n";
    cout << "  Type :quit to exit.\n";

    string input;

    while (true) {
        cout << "\nSearch prefix: ";

        if (!getline(cin, input))
            break;

        input = Trie::normalize(input);

        if (input == ":quit" || input == "exit")
            break;

        if (input == ":add") {
            cout << "Enter the query to add: ";

            string query;

            if (!getline(cin, query))
                break;

            query = Trie::normalize(query);

            if (query.empty()) {
                cout << "Query cannot be empty.\n";
                continue;
            }

            if (!searchTrie.contains(query)) {
                searchTrie.insert(query, 1);
                cout << "New query added.\n";
            }

            searchTrie.recordSelection(query);

            if (saveUserScores())
                cout << "Query saved for future runs.\n";
            else
                cout << "Warning: could not save the query.\n";

            continue;
        }

        if (input.empty()) {
            cout << "Please enter a prefix.\n";
            continue;
        }

        vector<Suggestion> results =
            getSuggestions(input);

        if (results.empty()) {
            cout << "No matching queries found.\n";
            cout << "Use :add to add a new query.\n";
            continue;
        }

        cout << "\nTop suggestions:\n";

        for (size_t i = 0; i < results.size(); ++i) {
            cout << i + 1 << ". "
                 << results[i].query
                 << " [score: "
                 << results[i].score
                 << "]\n";
        }

        cout << "\nChoose 1-"
             << results.size()
             << " to select a suggestion, or 0 to continue: ";

        string choiceText;

        if (!getline(cin, choiceText))
            break;

        long long choice = -1;

        if (!parseNumber(choiceText, choice)) {
            cout << "Invalid choice.\n";
            continue;
        }

        if (choice >= 1 &&
            choice <= static_cast<long long>(results.size())) {

            string selected =
                results[static_cast<size_t>(choice - 1)].query;

            searchTrie.recordSelection(selected);

            if (saveUserScores()) {
                cout << "Selection recorded for: "
                     << selected << "\n";
            } else {
                cout << "Warning: selection could not be saved.\n";
            }

        } else if (choice == 0) {
            cout << "No selection recorded.\n";
        } else {
            cout << "Invalid choice.\n";
        }
    }

    cout << "\nSearch engine closed.\n";

    return 0;
}
