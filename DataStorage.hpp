#ifndef DATA_STORAGE_HPP
#define DATA_STORAGE_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>

/**
 * A simple disk-based storage helper for fixed-size records.
 */
template <class T, int info_len = 2>
class DataStorage {
private:
    std::fstream file;
    std::string file_name;
    int sizeofT = sizeof(T);

public:
    DataStorage() = default;
    DataStorage(const std::string &fn) : file_name(fn) {
        file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) {
            file.open(file_name, std::ios::out | std::ios::binary);
            file.close();
            file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
            int zero = 0;
            for (int i = 0; i < info_len; ++i) {
                file.write(reinterpret_cast<char *>(&zero), sizeof(int));
            }
        }
    }

    void get_info(int &val, int index) {
        file.seekg(index * sizeof(int));
        file.read(reinterpret_cast<char *>(&val), sizeof(int));
    }

    void write_info(int val, int index) {
        file.seekp(index * sizeof(int));
        file.write(reinterpret_cast<char *>(&val), sizeof(int));
    }

    int write(T &t) {
        file.seekp(0, std::ios::end);
        int pos = file.tellp();
        file.write(reinterpret_cast<char *>(&t), sizeofT);
        return pos;
    }

    void update(T &t, const int pos) {
        file.seekp(pos);
        file.write(reinterpret_cast<char *>(&t), sizeofT);
    }

    void read(T &t, const int pos) {
        file.seekg(pos);
        file.read(reinterpret_cast<char *>(&t), sizeofT);
    }
};

template <int N>
struct FixedString {
    char data[N + 1];
    FixedString() { memset(data, 0, sizeof(data)); }
    FixedString(const std::string &s) {
        memset(data, 0, sizeof(data));
        strncpy(data, s.c_str(), N);
    }
    bool operator<(const FixedString &other) const { return strcmp(data, other.data) < 0; }
    bool operator>(const FixedString &other) const { return strcmp(data, other.data) > 0; }
    bool operator<=(const FixedString &other) const { return strcmp(data, other.data) <= 0; }
    bool operator>=(const FixedString &other) const { return strcmp(data, other.data) >= 0; }
    bool operator==(const FixedString &other) const { return strcmp(data, other.data) == 0; }
    bool operator!=(const FixedString &other) const { return strcmp(data, other.data) != 0; }
    std::string to_string() const { return std::string(data); }
};

#endif
