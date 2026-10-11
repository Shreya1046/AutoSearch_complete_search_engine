#ifndef TOPK_H
#define TOPK_H

#include <vector>
#include <string>
#include <algorithm>

using namespace std;

class TopK {
private:
    vector<pair<string, int>> heap;
    int capacity;

    // Returns true if a should rank higher than b.
    bool better(
        const pair<string, int>& a,
        const pair<string, int>& b
    ) const {
        if (a.second != b.second) {
            return a.second > b.second;
        }

        return a.first < b.first;
    }

    // Maintain the heap after insertion.
    void siftUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;

            // Stop when the parent is worse than or equal
            // to the child: the root must be the worst item.
            if (better(heap[parent], heap[index])) {
                swap(heap[parent], heap[index]);
            } else {
                break;
            }

            index = parent;
        }
    }

    // Maintain the heap after replacing its root.
    void siftDown(int index) {
        int size = static_cast<int>(heap.size());

        while (true) {
            int worst = index;
            int left = 2 * index + 1;
            int right = 2 * index + 2;

            // Find the worst-ranked element.
            if (left < size && better(heap[worst], heap[left])) {
                worst = left;
            }

            if (right < size && better(heap[worst], heap[right])) {
                worst = right;
            }

            if (worst == index) {
                break;
            }

            swap(heap[index], heap[worst]);
            index = worst;
        }
    }

public:
    explicit TopK(int k) : capacity(k) {}

    void add(const pair<string, int>& candidate) {
        if (capacity <= 0) {
            return;
        }

        if (static_cast<int>(heap.size()) < capacity) {
            heap.push_back(candidate);
            siftUp(static_cast<int>(heap.size()) - 1);
        } else if (better(candidate, heap[0])) {
            heap[0] = candidate;
            siftDown(0);
        }
    }

    vector<pair<string, int>> getResults() const {
        vector<pair<string, int>> results = heap;

        sort(
            results.begin(),
            results.end(),
            [this](
                const pair<string, int>& a,
                const pair<string, int>& b
            ) {
                return better(a, b);
            }
        );

        return results;
    }
};

#endif