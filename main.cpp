#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <climits>
#include <unordered_map>

const std::string STORAGE_DIR = "storage";

// Simple structure to hold index-value pairs
struct Entry {
    std::string index;
    int value;

    bool operator<(const Entry& other) const {
        if (index != other.index) return index < other.index;
        return value < other.value;
    }

    bool operator==(const Entry& other) const {
        return index == other.index && value == other.value;
    }
};

class FileStorage {
private:
    // Get filename based on first character of index
    std::string get_filename(const std::string& index) {
        if (index.empty()) return STORAGE_DIR + "/_.dat";
        char first = index[0];
        // Use alphanumeric characters directly, others go to special file
        if ((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z') || (first >= '0' && first <= '9')) {
            return STORAGE_DIR + "/" + first + ".dat";
        }
        return STORAGE_DIR + "/special.dat";
    }

    void ensure_directory() {
        struct stat st;
        if (stat(STORAGE_DIR.c_str(), &st) != 0) {
            mkdir(STORAGE_DIR.c_str(), 0755);
        }
    }

    std::vector<Entry> load_entries(const std::string& filename) {
        std::vector<Entry> entries;
        std::ifstream infile(filename);

        if (infile.is_open()) {
            std::string line;
            while (std::getline(infile, line)) {
                if (!line.empty()) {
                    std::istringstream iss(line);
                    Entry e;
                    if (iss >> e.index >> e.value) {
                        entries.push_back(e);
                    }
                }
            }
            infile.close();
        }

        // Sort entries
        std::sort(entries.begin(), entries.end());
        return entries;
    }

    void save_entries(const std::string& filename, const std::vector<Entry>& entries) {
        std::ofstream outfile(filename, std::ios::trunc);
        if (outfile.is_open()) {
            for (const Entry& e : entries) {
                outfile << e.index << " " << e.value << "\n";
            }
            outfile.close();
        }
    }

public:
    FileStorage() {
        ensure_directory();
    }

    void insert(const std::string& index, int value) {
        std::string filename = get_filename(index);
        std::vector<Entry> entries = load_entries(filename);

        // Check if entry already exists
        Entry new_entry{index, value};
        auto it = std::lower_bound(entries.begin(), entries.end(), new_entry);

        // Only insert if not found
        if (it == entries.end() || !(*it == new_entry)) {
            entries.insert(it, new_entry);
            save_entries(filename, entries);
        }
    }

    void delete_entry(const std::string& index, int value) {
        std::string filename = get_filename(index);
        std::vector<Entry> entries = load_entries(filename);
        Entry target{index, value};

        auto it = std::lower_bound(entries.begin(), entries.end(), target);
        if (it != entries.end() && *it == target) {
            entries.erase(it);
            save_entries(filename, entries);
        }
    }

    std::vector<int> find(const std::string& index) {
        std::string filename = get_filename(index);
        std::vector<Entry> entries = load_entries(filename);
        std::vector<int> result;

        // Find all entries with matching index
        Entry start{index, 0};
        Entry end{index, INT_MAX};

        auto start_it = std::lower_bound(entries.begin(), entries.end(), start);
        auto end_it = std::lower_bound(entries.begin(), entries.end(), end);

        for (auto it = start_it; it != end_it; ++it) {
            if (it->index == index) {
                result.push_back(it->value);
            }
        }

        return result;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    FileStorage storage;

    int n;
    std::cin >> n;
    std::cin.ignore();  // Consume newline

    for (int i = 0; i < n; i++) {
        std::string line;
        std::getline(std::cin, line);

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "insert") {
            std::string index;
            int value;
            iss >> index >> value;
            storage.insert(index, value);
        } else if (command == "delete") {
            std::string index;
            int value;
            iss >> index >> value;
            storage.delete_entry(index, value);
        } else if (command == "find") {
            std::string index;
            iss >> index;
            std::vector<int> values = storage.find(index);

            if (values.empty()) {
                std::cout << "null" << std::endl;
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << values[j];
                }
                std::cout << std::endl;
            }
        }
    }

    return 0;
}