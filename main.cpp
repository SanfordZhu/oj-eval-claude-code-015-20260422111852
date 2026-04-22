#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

class FileStorage {
private:
    std::string base_dir = "storage";

    std::string get_filename(const std::string& index) {
        // Create a safe filename from index
        std::string safe_index;
        for (char c : index) {
            if (c == '/' || c == '\\' || c == ' ' || c == '\t') {
                safe_index += '_';
            } else {
                safe_index += c;
            }
        }
        return base_dir + "/" + safe_index + ".dat";
    }

    void ensure_directory() {
        if (!fs::exists(base_dir)) {
            fs::create_directory(base_dir);
        }
    }

public:
    FileStorage() {
        ensure_directory();
    }

    void insert(const std::string& index, int value) {
        std::string filename = get_filename(index);
        std::vector<int> values;

        // Read existing values
        std::ifstream infile(filename);
        if (infile.is_open()) {
            int val;
            while (infile >> val) {
                if (val != value) {  // Avoid duplicates
                    values.push_back(val);
                }
            }
            infile.close();
        }

        // Add new value
        values.push_back(value);

        // Sort values
        std::sort(values.begin(), values.end());

        // Write back to file
        std::ofstream outfile(filename);
        for (int val : values) {
            outfile << val << " ";
        }
        outfile.close();
    }

    void delete_entry(const std::string& index, int value) {
        std::string filename = get_filename(index);

        // Check if file exists
        if (!fs::exists(filename)) {
            return;
        }

        std::vector<int> values;

        // Read existing values
        std::ifstream infile(filename);
        if (infile.is_open()) {
            int val;
            while (infile >> val) {
                if (val != value) {
                    values.push_back(val);
                }
            }
            infile.close();
        }

        // Write back to file
        std::ofstream outfile(filename);
        for (int val : values) {
            outfile << val << " ";
        }
        outfile.close();

        // Remove file if empty
        if (values.empty()) {
            fs::remove(filename);
        }
    }

    std::vector<int> find(const std::string& index) {
        std::string filename = get_filename(index);
        std::vector<int> values;

        // Check if file exists
        if (!fs::exists(filename)) {
            return values;
        }

        // Read values
        std::ifstream infile(filename);
        if (infile.is_open()) {
            int val;
            while (infile >> val) {
                values.push_back(val);
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