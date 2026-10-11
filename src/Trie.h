#ifndef TRIE_H
#define TRIE_H

#include <string>
#include <vector>
#include <climits>

struct QueryRecord {
    std::string query;
    long long score;
    long long selections;

    QueryRecord(const std::string& q, long long s)
        : query(q), score(s), selections(0) {}
};

class Trie {
private:
    struct Node {
        int firstChild;
        int nextSibling;
        int recordIndex;
        unsigned char ch;

        Node(unsigned char c = 0)
            : firstChild(-1),
              nextSibling(-1),
              recordIndex(-1),
              ch(c) {}
    };

    std::vector<Node> nodes;
    std::vector<QueryRecord> records;

    int findChild(int parent, unsigned char ch) const {
        int child = nodes[parent].firstChild;

        while (child != -1) {
            if (nodes[child].ch == ch)
                return child;

            child = nodes[child].nextSibling;
        }

        return -1;
    }

    int getOrCreateChild(int parent, unsigned char ch) {
        int child = findChild(parent, ch);

        if (child != -1)
            return child;

        int index = static_cast<int>(nodes.size());

        nodes.push_back(Node(ch));

        nodes[index].nextSibling = nodes[parent].firstChild;
        nodes[parent].firstChild = index;

        return index;
    }

    int findNode(const std::string& text) const {
        int current = 0;

        for (std::size_t i = 0; i < text.size(); ++i) {
            int child = findChild(
                current,
                static_cast<unsigned char>(text[i])
            );

            if (child == -1)
                return -1;

            current = child;
        }

        return current;
    }

    template <typename Callback>
    void traverse(int nodeIndex, Callback& callback) const {
        int index = nodes[nodeIndex].recordIndex;

        if (index != -1)
            callback(records[index]);

        int child = nodes[nodeIndex].firstChild;

        while (child != -1) {
            traverse(child, callback);
            child = nodes[child].nextSibling;
        }
    }

public:
    Trie() {
        nodes.push_back(Node());
    }

    static std::string normalize(const std::string& text) {
        std::string result;
        result.reserve(text.size());

        bool previousSpace = true;

        for (std::size_t i = 0; i < text.size(); ++i) {
            unsigned char ch =
                static_cast<unsigned char>(text[i]);

            if (ch == ' ' || ch == '\t' ||
                ch == '\r' || ch == '\n') {

                if (!previousSpace && !result.empty()) {
                    result.push_back(' ');
                    previousSpace = true;
                }
            } else {
                // Convert ASCII uppercase letters to lowercase.
                // Preserve symbols and UTF-8 bytes.
                if (ch >= 'A' && ch <= 'Z')
                    ch = ch - 'A' + 'a';

                result.push_back(static_cast<char>(ch));
                previousSpace = false;
            }
        }

        if (!result.empty() && result.back() == ' ')
            result.erase(result.size() - 1);

        return result;
    }

    bool insert(const std::string& rawQuery,
                long long initialScore = 1) {

        std::string query = normalize(rawQuery);

        if (query.empty())
            return false;

        int current = 0;

        for (std::size_t i = 0; i < query.size(); ++i) {
            current = getOrCreateChild(
                current,
                static_cast<unsigned char>(query[i])
            );
        }

        int existing = nodes[current].recordIndex;

        if (existing != -1) {
            if (initialScore > records[existing].score)
                records[existing].score = initialScore;

            return false;
        }

        int index = static_cast<int>(records.size());

        records.push_back(
            QueryRecord(
                query,
                initialScore > 0 ? initialScore : 1
            )
        );

        nodes[current].recordIndex = index;

        return true;
    }

    bool contains(const std::string& rawQuery) const {
        std::string query = normalize(rawQuery);

        if (query.empty())
            return false;

        int index = findNode(query);

        return index != -1 &&
               nodes[index].recordIndex != -1;
    }

    bool recordSelection(const std::string& rawQuery) {
        std::string query = normalize(rawQuery);

        if (query.empty())
            return false;

        int index = findNode(query);

        if (index == -1 ||
            nodes[index].recordIndex == -1) {

            insert(query, 1);
            index = findNode(query);
        }

        int recordIndex = nodes[index].recordIndex;

        if (records[recordIndex].score < LLONG_MAX)
            ++records[recordIndex].score;

        if (records[recordIndex].selections < LLONG_MAX)
            ++records[recordIndex].selections;

        return true;
    }

    void applySavedSelections(
        const std::string& rawQuery,
        long long count
    ) {
        std::string query = normalize(rawQuery);

        if (query.empty() || count <= 0)
            return;

        int index = findNode(query);

        if (index == -1 ||
            nodes[index].recordIndex == -1) {

            insert(query, 1);
            index = findNode(query);
        }

        int recordIndex = nodes[index].recordIndex;

        records[recordIndex].selections = count;

        if (records[recordIndex].score <= LLONG_MAX - count)
            records[recordIndex].score += count;
        else
            records[recordIndex].score = LLONG_MAX;
    }

    template <typename Callback>
    void forEachMatch(
        const std::string& rawPrefix,
        Callback callback
    ) const {
        std::string prefix = normalize(rawPrefix);

        int index = findNode(prefix);

        if (index == -1)
            return;

        traverse(index, callback);
    }

    const std::vector<QueryRecord>& getRecords() const {
        return records;
    }

    std::size_t queryCount() const {
        return records.size();
    }
};

#endif