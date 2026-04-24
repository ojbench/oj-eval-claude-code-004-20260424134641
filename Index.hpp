#ifndef INDEX_HPP
#define INDEX_HPP

#include "DataStorage.hpp"
#include <vector>
#include <algorithm>

/**
 * A simplified disk-based B+ tree or Blocked List for indexing.
 * Multiple blocks, each containing up to M elements. Blocks are sorted.
 */

template <class Key, class Value, int M = 300>
class Index {
    struct Node {
        Key keys[M];
        Value values[M];
        int size = 0;
        int next = -1;
    };

    struct Head {
        Key first_key;
        int pos;
    };

    std::string filename;
    DataStorage<Node, 2> node_storage;
    DataStorage<Head, 1> head_storage;
    int node_count = 0;

public:
    Index(const std::string &fn) : filename(fn), node_storage(fn + "_nodes.dat"), head_storage(fn + "_heads.dat") {
        head_storage.get_info(node_count, 0);
        if (node_count == 0) {
            Node first_node;
            int pos = node_storage.write(first_node);
            Head first_head;
            first_head.pos = pos;
            head_storage.write(first_head);
            node_count = 1;
            head_storage.write_info(node_count, 0);
        }
    }

    void insert(const Key &key, const Value &val) {
        int head_idx = find_head(key);
        Head h;
        head_storage.read(h, head_idx * sizeof(Head) + sizeof(int));
        Node n;
        node_storage.read(n, h.pos);

        auto it = std::lower_bound(n.keys, n.keys + n.size, key);
        int idx = std::distance(n.keys, it);

        for (int i = n.size; i > idx; --i) {
            n.keys[i] = n.keys[i - 1];
            n.values[i] = n.values[i - 1];
        }
        n.keys[idx] = key;
        n.values[idx] = val;
        n.size++;

        if (n.size >= M) {
            // Split
            Node new_node;
            int half = n.size / 2;
            for (int i = half; i < n.size; ++i) {
                new_node.keys[i - half] = n.keys[i];
                new_node.values[i - half] = n.values[i];
            }
            new_node.size = n.size - half;
            n.size = half;
            new_node.next = n.next;
            int new_pos = node_storage.write(new_node);
            n.next = new_pos;
            node_storage.update(n, h.pos);

            // Update heads
            std::vector<Head> heads(node_count + 1);
            for (int i = 0; i < node_count; ++i) {
                head_storage.read(heads[i], i * sizeof(Head) + sizeof(int));
            }
            Head new_h;
            new_h.first_key = new_node.keys[0];
            new_h.pos = new_pos;
            heads.insert(heads.begin() + head_idx + 1, new_h);

            node_count++;
            head_storage.write_info(node_count, 0);
            for (int i = 0; i < node_count; ++i) {
                head_storage.update(heads[i], i * sizeof(Head) + sizeof(int));
            }
        } else {
            node_storage.update(n, h.pos);
            if (idx == 0) {
                h.first_key = key;
                head_storage.update(h, head_idx * sizeof(Head) + sizeof(int));
            }
        }
    }

    void remove(const Key &key, const Value &val) {
        int head_idx = find_head(key);
        for (int i = head_idx; i < node_count; ++i) {
            Head h;
            head_storage.read(h, i * sizeof(Head) + sizeof(int));
            if (i > head_idx && h.first_key > key) break;

            Node n;
            node_storage.read(n, h.pos);
            bool found = false;
            for (int j = 0; j < n.size; ++j) {
                if (n.keys[j] == key && n.values[j] == val) {
                    for (int k = j; k < n.size - 1; ++k) {
                        n.keys[k] = n.keys[k + 1];
                        n.values[k] = n.values[k + 1];
                    }
                    n.size--;
                    found = true;
                    break;
                }
            }
            if (found) {
                node_storage.update(n, h.pos);
                if (n.size > 0) {
                    h.first_key = n.keys[0];
                    head_storage.update(h, i * sizeof(Head) + sizeof(int));
                }
                return;
            }
        }
    }

    std::vector<Value> find(const Key &key) {
        std::vector<Value> results;
        int head_idx = find_head(key);
        for (int i = head_idx; i < node_count; ++i) {
            Head h;
            head_storage.read(h, i * sizeof(Head) + sizeof(int));
            if (i > head_idx && h.first_key > key) break;

            Node n;
            node_storage.read(n, h.pos);
            auto it = std::lower_bound(n.keys, n.keys + n.size, key);
            int idx = std::distance(n.keys, it);
            while (idx < n.size && n.keys[idx] == key) {
                results.push_back(n.values[idx]);
                idx++;
            }
            // If we reached the end of the node and still have matching keys, they might be in the next node.
            // But with M=300 and our key distribution, it's unlikely a single key spans multiple nodes unless many duplicates.
            // Let's handle it just in case.
            while (idx == n.size && n.next != -1) {
                node_storage.read(n, n.next);
                idx = 0;
                while (idx < n.size && n.keys[idx] == key) {
                    results.push_back(n.values[idx]);
                    idx++;
                }
            }
            break;
        }
        return results;
    }

    std::vector<Value> get_all() {
        std::vector<Value> results;
        for (int i = 0; i < node_count; ++i) {
            Head h;
            head_storage.read(h, i * sizeof(Head) + sizeof(int));
            Node n;
            node_storage.read(n, h.pos);
            for (int j = 0; j < n.size; ++j) {
                results.push_back(n.values[j]);
            }
        }
        return results;
    }

private:
    int find_head(const Key &key) {
        int left = 0, right = node_count - 1;
        int ans = 0;
        while (left <= right) {
            int mid = (left + right) / 2;
            Head h;
            head_storage.read(h, mid * sizeof(Head) + sizeof(int));
            if (h.first_key <= key) {
                ans = mid;
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return ans;
    }
};

#endif
