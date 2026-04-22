#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <unordered_map>

// Use a fixed number of bucket files to limit file count
const int NUM_BUCKETS = 10;
const std::string BASE_DIR = "storage";

class FileStorage {
private:
    // Simple hash function to distribute indices across buckets
    int get_bucket(const std::string& index) {
        unsigned long hash = 0;
        for (char c : index) {
            hash = hash * 31 + c;
        }
        return hash % NUM_BUCKETS;
    }

    std::string get_bucket_filename(int bucket) {
        return BASE_DIR + "/bucket_" + std::to_string(bucket) + ".dat";
    }

    void ensure_directory() {
        std::ifstream dir_check(BASE_DIR + "/.exists");
        if (!dir_check.good()) {
            std::system(("mkdir -p " + BASE_DIR).c_str());
            std::ofstream marker(BASE_DIR + "/.exists");
            marker.close();
        }
    }

    // Format: index:value1,value2,value3\n
    void update_bucket(int bucket, const std::string& index, const std::vector<int>& values) {
        std::string bucket_file = get_bucket_filename(bucket);
        std::vector<std::string> all_entries;

        // Read existing entries
        std::ifstream infile(bucket_file);
        if (infile.is_open()) {
            std::string line;
            while (std::getline(infile, line)) {
                if (!line.empty()) {
                    size_t colon_pos = line.find(':');
                    if (colon_pos != std::string::npos) {
                        std::string existing_index = line.substr(0, colon_pos);
                        if (existing_index != index) {
                            all_entries.push_back(line);
                        }
                    }
                }
            }
            infile.close();
        }

        // Add updated entry if it has values
        if (!values.empty()) {
            std::string new_line = index + ":";
            for (size_t i = 0; i < values.size(); i++) {
                if (i > 0) new_line += ",";
                new_line += std::to_string(values[i]);
            }
            all_entries.push_back(new_line);
        }

        // Write back to file - use binary mode for consistency
        std::ofstream outfile(bucket_file, std::ios::trunc);
        if (outfile.is_open()) {
            for (const std::string& entry : all_entries) {
                outfile << entry << "\n";
            }
            outfile.close();
        }
    }

public:
    FileStorage() {
        ensure_directory();
    }

    void insert(const std::string& index, int value) {
        int bucket = get_bucket(index);
        std::vector<int> values = find_internal(bucket, index);

        // Add value if not already present
        if (std::find(values.begin(), values.end(), value) == values.end()) {
            values.push_back(value);
            std::sort(values.begin(), values.end());
            update_bucket(bucket, index, values);
        }
    }

    void delete_entry(const std::string& index, int value) {
        int bucket = get_bucket(index);
        std::vector<int> values = find_internal(bucket, index);

        // Remove value if present
        auto it = std::find(values.begin(), values.end(), value);
        if (it != values.end()) {
            values.erase(it);
            update_bucket(bucket, index, values);
        }
    }

    std::vector<int> find(const std::string& index) {
        int bucket = get_bucket(index);
        return find_internal(bucket, index);
    }

private:
    std::vector<int> find_internal(int bucket, const std::string& index) {
        std::vector<int> values;
        std::string bucket_file = get_bucket_filename(bucket);

        std::ifstream infile(bucket_file);
        if (infile.is_open()) {
            std::string line;
            while (std::getline(infile, line)) {
                // Skip empty lines
                if (line.empty()) continue;

                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string existing_index = line.substr(0, colon_pos);
                    if (existing_index == index) {
                        std::string values_str = line.substr(colon_pos + 1);
                        if (!values_str.empty()) {
                            std::stringstream ss(values_str);
                            std::string value_str;
                            while (std::getline(ss, value_str, ',')) {
                                if (!value_str.empty()) {
                                    values.push_back(std::stoi(value_str));
                                }
                            }
                        }
                        break;
                    }
                }
            }
            infile.close();
        }

        return values;
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