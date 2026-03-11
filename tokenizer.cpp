#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>

class Tokenizer {
private:
    int min_length;
    bool lowercase;
    int total_tokens;
    int total_chars;
    std::map<std::string, int> token_freq;
    std::set<std::string> unique_tokens;

public:
    Tokenizer(int min_len = 2, bool lower = true) 
        : min_length(min_len), lowercase(lower), total_tokens(0), total_chars(0) {}

    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::string current;

        for (size_t i = 0; i < text.length(); ++i) {
            char c = text[i];

            if (isAlphaNumeric(c)) {
                if (lowercase && c >= 'A' && c <= 'Z') {
                    current += static_cast<char>(c + 32);
                } else {
                    current += c;
                }
            } else {
                if (static_cast<int>(current.length()) >= min_length) {
                    tokens.push_back(current);
                    total_tokens++;
                    total_chars += current.length();
                    token_freq[current]++;
                    unique_tokens.insert(current);
                }
                current.clear();
            }
        }

        if (static_cast<int>(current.length()) >= min_length) {
            tokens.push_back(current);
            total_tokens++;
            total_chars += current.length();
            token_freq[current]++;
            unique_tokens.insert(current);
        }

        return tokens;
    }

    bool isAlphaNumeric(char c) {
        return (c >= 'a' && c <= 'z') || 
               (c >= 'A' && c <= 'Z') || 
               (c >= '0' && c <= '9');
    }

    void processFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filepath << std::endl;
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        tokenize(content);
    }

    void processMongoDB() {
        std::ifstream file("corpus_export.json");
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open corpus_export.json" << std::endl;
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        tokenize(content);
    }

    void printStatistics() {
        std::cout << "Tokenization Statistics:" << std::endl;
        std::cout << "Total tokens: " << total_tokens << std::endl;
        std::cout << "Unique tokens: " << unique_tokens.size() << std::endl;
        std::cout << "Total characters: " << total_chars << std::endl;
        
        if (total_tokens > 0) {
            double avg_length = static_cast<double>(total_chars) / total_tokens;
            std::cout << "Average token length: " << std::fixed << std::setprecision(2) << avg_length << std::endl;
        }

        std::cout << "\nTop 20 tokens by frequency:" << std::endl;
        
        std::vector<std::pair<std::string, int>> sorted_tokens(token_freq.begin(), token_freq.end());
        std::sort(sorted_tokens.begin(), sorted_tokens.end(), 
            [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                return a.second > b.second;
            });

        for (size_t i = 0; i < 20 && i < sorted_tokens.size(); ++i) {
            std::cout << (i + 1) << ". " << sorted_tokens[i].first 
                      << ": " << sorted_tokens[i].second << std::endl;
        }
    }

    void saveTokenFreq(const std::string& filepath) {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create file " << filepath << std::endl;
            return;
        }

        std::vector<std::pair<std::string, int>> sorted_tokens(token_freq.begin(), token_freq.end());
        std::sort(sorted_tokens.begin(), sorted_tokens.end(), 
            [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                return a.second > b.second;
            });

        for (const auto& pair : sorted_tokens) {
            file << pair.first << "\t" << pair.second << "\n";
        }

        file.close();
    }

    int getTotalTokens() const { return total_tokens; }
    int getTotalChars() const { return total_chars; }
    int getUniqueTokens() const { return unique_tokens.size(); }
};

int main(int argc, char* argv[]) {
    auto start = std::chrono::high_resolution_clock::now();

    Tokenizer tokenizer(2, true);

    if (argc > 1) {
        std::string input_file = argv[1];
        std::cout << "Processing file: " << input_file << std::endl;
        tokenizer.processFile(input_file);
    } else {
        std::cout << "Processing MongoDB export..." << std::endl;
        tokenizer.processMongoDB();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    tokenizer.printStatistics();
    tokenizer.saveTokenFreq("token_freq.txt");

    std::cout << "\nExecution time: " << duration.count() << " ms" << std::endl;

    if (tokenizer.getTotalChars() > 0) {
        double speed_kb = (tokenizer.getTotalChars() / 1024.0) / (duration.count() / 1000.0);
        std::cout << "Tokenization speed: " << std::fixed << std::setprecision(2) 
                  << speed_kb << " KB/sec" << std::endl;
    }

    return 0;
}