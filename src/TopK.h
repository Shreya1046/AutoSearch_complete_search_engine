#ifndef TOPK_H
#define TOPK_H

#include <string>
#include <vector>
#include <algorithm>

struct Suggestion {
    std::string query;
    long long score;

    Suggestion(const std::string& q, long long s)
        : query(q), score(s) {}
};

class TopK {
private:
    std::vector<Suggestion> heap;
    std::size_t capacity;

    static bool better(
        const Suggestion& a,
        const Suggestion& b
    ) {
        if (a.score != b.score)
            return a.score > b.score;

        return a.query < b.query;
    }

    // The root stores the worst suggestion currently retained.
    void siftUp(std::size_t index) {
        while (index > 0) {
            std::size_t parent = (index - 1) / 2;

            if (better(heap[parent], heap[index])) {
                std::swap(heap[parent], heap[index]);
                index = parent;
            } else {
                break;
            }
        }
    }

    void siftDown(std::size_t index) {
        while (true) {
            std::size_t worst = index;
            std::size_t left = 2 * index + 1;
            std::size_t right = 2 * index + 2;

            if (left < heap.size() &&
                better(heap[worst], heap[left])) {
                worst = left;
            }

            if (right < heap.size() &&
                better(heap[worst], heap[right])) {
                worst = right;
            }

            if (worst == index)
                break;

            std::swap(heap[index], heap[worst]);
            index = worst;
        }
    }

public:
    explicit TopK(std::size_t k)
        : capacity(k) {
        heap.reserve(k);
    }

    void add(const Suggestion& suggestion) {
        if (capacity == 0)
            return;

        if (heap.size() < capacity) {
            heap.push_back(suggestion);
            siftUp(heap.size() - 1);
        } else if (better(suggestion, heap[0])) {
            heap[0] = suggestion;
            siftDown(0);
        }
    }

    std::vector<Suggestion> getResults() const {
        std::vector<Suggestion> results = heap;

        std::sort(
            results.begin(),
            results.end(),
            better
        );

        return results;
    }
};

#endif