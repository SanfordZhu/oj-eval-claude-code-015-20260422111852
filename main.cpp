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

class FileStorage {
private:
    // Get filename based on hash of index
    std::string get_filename(const std::string& index) {
        unsigned long hash = 0;
        for (char c : index) {
            hash = hash * 31 + c;
        }
        return STORAGE_DIR + "/bucket_" + std::to_string(hash % 37) + ".dat";  // 37 buckets
    }

    void ensure_directory() {
        struct stat st;
        if (stat(STORAGE_DIR.c_str(), &st) != 0) {
            mkdir(STORAGE_DIR.c_str(), 0755);
        }
    }

    // Binary search in a sorted file
    bool binary_search_in_file(const std::string& filename, const std::string& index, int value, std::vector<int>& found_values) {
        std::ifstream infile(filename, std::ios::binary);
        if (!infile.is_open()) return false;

        // Get file size
        infile.seekg(0, std::ios::end);
        std::streamsize file_size = infile.tellg();
        infile.seekg(0, std::ios::beg);

        if (file_size == 0) return false;

        // Each record: index (64 bytes) + value (4 bytes) = 68 bytes
        const int RECORD_SIZE = 68;
        int left = 0;
        int right = (file_size / RECORD_SIZE) - 1;

        bool found = false;

        while (left <= right) {
            int mid = left + (right - left) / 2;
            infile.seekg(mid * RECORD_SIZE);

            char buffer[65] = {0};
            int val;
            infile.read(buffer, 64);
            infile.read(reinterpret_cast<char*>(&val), 4);

            std::string current_index(buffer);
            // Remove padding
            size_t null_pos = current_index.find('\0');
            if (null_pos != std::string::npos) {
                current_index = current_index.substr(0, null_pos);
            }

            if (current_index == index) {
                // Found a match, now find all entries with this index
                found = true;

                // Search backwards for first occurrence
                int first = mid;
                while (first > left) {
                    infile.seekg((first - 1) * RECORD_SIZE);
                    char temp_buffer[65] = {0};
                    infile.read(temp_buffer, 64);
                    std::string temp_index(temp_buffer);
                    size_t null_pos_temp = temp_index.find('\0');
                    if (null_pos_temp != std::string::npos) {
                        temp_index = temp_index.substr(0, null_pos_temp);
                    }
                    if (temp_index == index) {
                        first--;
                    } else {
                        break;
                    }
                }

                // Collect all values for this index
                int current = first;
                while (current <= right) {
                    infile.seekg(current * RECORD_SIZE);
                    char collect_buffer[65] = {0};
                    int collect_val;
                    infile.read(collect_buffer, 64);
                    infile.read(reinterpret_cast<char*>(&collect_val), 4);

                    std::string collect_index(collect_buffer);
                    size_t null_pos_collect = collect_index.find('\0');
                    if (null_pos_collect != std::string::npos) {
                        collect_index = collect_index.substr(0, null_pos_collect);
                    }

                    if (collect_index == index) {
                        found_values.push_back(collect_val);
                        current++;
                    } else {
                        break;
                    }
                }

                break;
            } else if (current_index < index) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }

        infile.close();
        return found;
    }

    void insert_to_file(const std::string& filename, const std::string& index, int value) {
        // First, load all entries
        std::vector<std::pair<std::string, int>> entries;
        std::ifstream infile(filename, std::ios::binary);

        if (infile.is_open()) {
            const int RECORD_SIZE = 68;
            char buffer[65];
            int val;

            while (infile.read(buffer, 64)) {
                infile.read(reinterpret_cast<char*>(&val), 4);
                std::string idx(buffer);
                size_t null_pos = idx.find('\0');
                if (null_pos != std::string::npos) {
                    idx = idx.substr(0, null_pos);
                }
                entries.emplace_back(idx, val);
            }
            infile.close();
        }

        // Add new entry
        entries.emplace_back(index, value);

        // Sort entries
        std::sort(entries.begin(), entries.end());

        // Write back to file
        std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
        if (outfile.is_open()) {
            for (const auto& e : entries) {
                char buffer[64] = {0};
                std::strncpy(buffer, e.first.c_str(), 63);
                outfile.write(buffer, 64);
                outfile.write(reinterpret_cast<const char*>(&e.second), 4);
            }
            outfile.close();
        }
    }

    void delete_from_file(const std::string& filename, const std::string& index, int value) {
        // Load all entries except the one to delete
        std::vector<std::pair<std::string, int>> entries;
        std::ifstream infile(filename, std::ios::binary);

        if (infile.is_open()) {
            const int RECORD_SIZE = 68;
            char buffer[65];
            int val;

            while (infile.read(buffer, 64)) {
                infile.read(reinterpret_cast<char*>(&val), 4);
                std::string idx(buffer);
                size_t null_pos = idx.find('\0');
                if (null_pos != std::string::npos) {
                    idx = idx.substr(0, null_pos);
                }

                // Skip the entry to delete
                if (idx == index && val == value) {
                    continue;
                }
                entries.emplace_back(idx, val);
            }
            infile.close();
        }

        // Write back to file
        std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
        if (outfile.is_open()) {
            for (const auto& e : entries) {
                char buffer[64] = {0};
                std::strncpy(buffer, e.first.c_str(), 63);
                outfile.write(buffer, 64);
                outfile.write(reinterpret_cast<const char*>(&e.second), 4);
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

        // First check if it already exists
        std::vector<int> found;
        if (binary_search_in_file(filename, index, value, found)) {
            if (std::find(found.begin(), found.end(), value) != found.end()) {
                return;  // Already exists
            }
        }

        insert_to_file(filename, index, value);
    }

    void delete_entry(const std::string& index, int value) {
        std::string filename = get_filename(index);
        delete_from_file(filename, index, value);
    }

    std::vector<int> find(const std::string& index) {
        std::string filename = get_filename(index);
        std::vector<int> result;
        binary_search_in_file(filename, index, 0, result);
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