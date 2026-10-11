#ifndef TRIE_H
#define TRIE_H

#include <string>
#include <vector>
#include <cctype>

using namespace std;

class TrieNode {
public:
    TrieNode* children[27];
    bool isEndOfQuery;
    int frequency;

    TrieNode() {
        isEndOfQuery = false;
        frequency = 0;

        for (int i = 0; i < 27; i++) {
            children[i] = nullptr;
        }
    }
};

class Trie {
private:
    TrieNode* root;

    string toLowerCase(const string& text) {
        string result = text;

        for (char& ch : result) {
            ch = static_cast<char>(
                tolower(static_cast<unsigned char>(ch))
            );
        }

        return result;
    }

    int getIndex(char ch) {
        if (ch >= 'a' && ch <= 'z') {
            return ch - 'a';
        }

        if (ch == ' ') {
            return 26;
        }

        return -1;
    }

    TrieNode* searchPrefix(const string& prefix) {
        TrieNode* current = root;

        for (char ch : prefix) {
            int index = getIndex(ch);

            if (index == -1 ||
                current->children[index] == nullptr) {
                return nullptr;
            }

            current = current->children[index];
        }

        return current;
    }

    void collectWords(
        TrieNode* node,
        string& currentWord,
        vector<pair<string, int>>& results
    ) {
        if (node == nullptr) {
            return;
        }

        if (node->isEndOfQuery) {
            results.push_back({currentWord, node->frequency});
        }

        for (int i = 0; i < 27; i++) {
            if (node->children[i] != nullptr) {
                if (i == 26) {
                    currentWord.push_back(' ');
                } else {
                    currentWord.push_back('a' + i);
                }

                collectWords(
                    node->children[i],
                    currentWord,
                    results
                );

                currentWord.pop_back();
            }
        }
    }

public:
    Trie() {
        root = new TrieNode();
    }

    void insert(const string& query, int frequency) {
        string normalizedQuery = toLowerCase(query);
        TrieNode* current = root;

        for (char ch : normalizedQuery) {
            int index = getIndex(ch);

            if (index == -1) {
                return;
            }

            if (current->children[index] == nullptr) {
                current->children[index] = new TrieNode();
            }

            current = current->children[index];
        }

        current->isEndOfQuery = true;
        current->frequency = frequency;
    }

    vector<pair<string, int>> autocomplete(const string& prefix) {
        vector<pair<string, int>> results;

        string normalizedPrefix = toLowerCase(prefix);

        TrieNode* prefixNode = searchPrefix(normalizedPrefix);

        if (prefixNode == nullptr) {
            return results;
        }

        string currentWord = normalizedPrefix;

        collectWords(prefixNode, currentWord, results);

        return results;
    }
};

#endif