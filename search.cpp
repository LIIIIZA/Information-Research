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
#include <iomanip>
#include <cstdint>
#include <cstring>

struct TermRecord {
    std::string term;
    std::vector<int> doc_ids;
    std::vector<int> frequencies;
};

struct DocRecord {
    int doc_id;
    std::string title;
    std::string url;
    std::string source;
};

class SearchEngine {
private:
    std::map<std::string, TermRecord> inverted_index;
    std::map<int, DocRecord> forward_index;
    int doc_count;

    std::string readString(std::ifstream& file) {
        uint32_t length;
        file.read(reinterpret_cast<char*>(&length), sizeof(length));
        if (length == 0) {
            return "";
        }
        std::string str(length, '\0');
        file.read(&str[0], length);
        return str;
    }

    std::vector<std::string> tokenizeQuery(const std::string& query) {
        std::vector<std::string> terms;
        std::string current;

        for (size_t i = 0; i < query.length(); ++i) {
            char c = query[i];
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                current += c;
            } else {
                if (current.length() >= 2) {
                    terms.push_back(current);
                }
                current.clear();
            }
        }

        if (current.length() >= 2) {
            terms.push_back(current);
        }

        return terms;
    }

    std::string toLower(const std::string& str) {
        std::string result = str;
        for (size_t i = 0; i < result.length(); ++i) {
            if (result[i] >= 'A' && result[i] <= 'Z') {
                result[i] = result[i] + 32;
            }
        }
        return result;
    }

    std::set<int> evaluateAnd(const std::vector<std::string>& terms) {
        if (terms.empty()) {
            return std::set<int>();
        }

        std::map<std::string, TermRecord>::iterator it = inverted_index.find(terms[0]);
        if (it == inverted_index.end()) {
            return std::set<int>();
        }

        std::set<int> result(it->second.doc_ids.begin(), it->second.doc_ids.end());

        for (size_t i = 1; i < terms.size(); ++i) {
            it = inverted_index.find(terms[i]);
            if (it == inverted_index.end()) {
                return std::set<int>();
            }

            std::set<int> intersection;
            for (size_t j = 0; j < it->second.doc_ids.size(); ++j) {
                if (result.find(it->second.doc_ids[j]) != result.end()) {
                    intersection.insert(it->second.doc_ids[j]);
                }
            }
            result = intersection;
        }

        return result;
    }

    std::set<int> evaluateOr(const std::vector<std::string>& terms) {
        std::set<int> result;

        for (size_t i = 0; i < terms.size(); ++i) {
            std::map<std::string, TermRecord>::iterator it = inverted_index.find(terms[i]);
            if (it != inverted_index.end()) {
                for (size_t j = 0; j < it->second.doc_ids.size(); ++j) {
                    result.insert(it->second.doc_ids[j]);
                }
            }
        }

        return result;
    }

    std::set<int> evaluateNot(const std::string& term) {
        std::set<int> result;
        std::map<std::string, TermRecord>::iterator it = inverted_index.find(term);

        for (std::map<int, DocRecord>::iterator doc_it = forward_index.begin();
             doc_it != forward_index.end(); ++doc_it) {
            if (it == inverted_index.end()) {
                result.insert(doc_it->first);
            } else {
                bool found = false;
                for (size_t i = 0; i < it->second.doc_ids.size(); ++i) {
                    if (it->second.doc_ids[i] == doc_it->first) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    result.insert(doc_it->first);
                }
            }
        }

        return result;
    }

public:
    SearchEngine() : doc_count(0) {}

    bool loadIndex(const std::string& inverted_path, const std::string& forward_path) {
        std::ifstream inv_file(inverted_path.c_str(), std::ios::binary);
        if (!inv_file.is_open()) {
            std::cerr << "Error: Cannot open " << inverted_path << std::endl;
            return false;
        }

        int term_count;
        inv_file.read(reinterpret_cast<char*>(&term_count), sizeof(term_count));

        for (int i = 0; i < term_count; ++i) {
            std::string term = readString(inv_file);

            int doc_count;
            inv_file.read(reinterpret_cast<char*>(&doc_count), sizeof(doc_count));

            TermRecord record;
            record.term = term;

            for (int j = 0; j < doc_count; ++j) {
                int doc_id, freq;
                inv_file.read(reinterpret_cast<char*>(&doc_id), sizeof(int));
                inv_file.read(reinterpret_cast<char*>(&freq), sizeof(int));
                record.doc_ids.push_back(doc_id);
                record.frequencies.push_back(freq);
            }

            inverted_index[term] = record;
        }
        inv_file.close();

        std::ifstream fwd_file(forward_path.c_str(), std::ios::binary);
        if (!fwd_file.is_open()) {
            std::cerr << "Error: Cannot open " << forward_path << std::endl;
            return false;
        }

        fwd_file.read(reinterpret_cast<char*>(&this->doc_count), sizeof(this->doc_count));

        for (int i = 0; i < this->doc_count; ++i) {
            DocRecord doc;
            fwd_file.read(reinterpret_cast<char*>(&doc.doc_id), sizeof(int));
            doc.title = readString(fwd_file);
            doc.url = readString(fwd_file);
            doc.source = readString(fwd_file);
            forward_index[doc.doc_id] = doc;
        }
        fwd_file.close();

        return true;
    }

    void search(const std::string& query) {
        auto start = std::chrono::high_resolution_clock::now();

        std::string lower_query = toLower(query);
        std::vector<std::string> terms = tokenizeQuery(lower_query);

        std::set<int> results;
        std::string op = "AND";

        if (lower_query.find("||") != std::string::npos || 
            lower_query.find(" or ") != std::string::npos) {
            op = "OR";
        } else if (lower_query.find("!") != std::string::npos || 
                   lower_query.find(" not ") != std::string::npos) {
            op = "NOT";
        }

        if (op == "OR") {
            results = evaluateOr(terms);
        } else if (op == "NOT") {
            if (!terms.empty()) {
                results = evaluateNot(terms[0]);
            }
        } else {
            results = evaluateAnd(terms);
        }

        auto end = std::chrono::high_resolution_clock::now();
        double search_time = std::chrono::duration<double>(end - start).count();

        std::cout << std::endl;
        std::cout << "Documents found: " << results.size() << std::endl;
        std::cout << "Search time: " << std::fixed << std::setprecision(4) 
                  << search_time << " sec" << std::endl;
        std::cout << std::endl;

        if (!results.empty()) {
            std::cout << "Results (top 20):" << std::endl;
            int count = 0;
            for (std::set<int>::iterator it = results.begin(); 
                 it != results.end() && count < 20; ++it, ++count) {
                std::map<int, DocRecord>::iterator doc_it = forward_index.find(*it);
                if (doc_it != forward_index.end()) {
                    std::cout << "  " << (count + 1) << ". [" << doc_it->second.source << "] " 
                              << doc_it->second.title << std::endl;
                    std::cout << "      URL: " << doc_it->second.url << std::endl;
                }
            }
            if (results.size() > 20) {
                std::cout << "  ... and " << (results.size() - 20) << " more documents" << std::endl;
            }
        } else {
            std::cout << "  No documents found" << std::endl;
        }
        std::cout << std::endl;
    }

    void showStats() {
        int total_postings = 0;
        for (std::map<std::string, TermRecord>::iterator it = inverted_index.begin();
             it != inverted_index.end(); ++it) {
            total_postings += static_cast<int>(it->second.doc_ids.size());
        }

        std::cout << std::endl;
        std::cout << "Index Statistics:" << std::endl;
        std::cout << "  Documents: " << doc_count << std::endl;
        std::cout << "  Terms: " << inverted_index.size() << std::endl;
        std::cout << "  Postings: " << total_postings << std::endl;
        std::cout << std::endl;
    }

    void showTerms() {
        std::vector<std::string> terms;
        for (std::map<std::string, TermRecord>::iterator it = inverted_index.begin();
             it != inverted_index.end(); ++it) {
            terms.push_back(it->first);
        }
        std::sort(terms.begin(), terms.end());

        std::cout << std::endl;
        std::cout << "Terms (first 50):" << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        int count = 0;
        for (size_t i = 0; i < terms.size() && count < 50; ++i, ++count) {
            std::cout << "  " << terms[i];
            if (count % 5 == 4) {
                std::cout << std::endl;
            }
        }
        std::cout << std::endl << std::endl;
    }

    void interactiveMode() {
        std::cout << "============================================================" << std::endl;
        std::cout << "Boolean Search - Interactive Mode" << std::endl;
        std::cout << "============================================================" << std::endl;
        std::cout << std::endl;
        std::cout << "Supported operators:" << std::endl;
        std::cout << "  AND  - both terms required (default)" << std::endl;
        std::cout << "  OR   - at least one term" << std::endl;
        std::cout << "  NOT  - exclude term" << std::endl;
        std::cout << std::endl;
        std::cout << "Example queries:" << std::endl;
        std::cout << "  machine learning" << std::endl;
        std::cout << "  python AND security" << std::endl;
        std::cout << "  ai OR artificial intelligence" << std::endl;
        std::cout << "  NOT game" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands: stats, terms, help, quit" << std::endl;
        std::cout << "============================================================" << std::endl;
        std::cout << std::endl;

        char buffer[1024];

        while (true) {
            std::cout << "Query: ";
            std::cin.getline(buffer, sizeof(buffer));

            std::string query(buffer);

            if (query.empty()) {
                continue;
            }

            if (query == "quit" || query == "exit") {
                std::cout << "Goodbye!" << std::endl;
                break;
            }

            if (query == "help") {
                std::cout << std::endl;
                std::cout << "Operators: AND, OR, NOT" << std::endl;
                std::cout << "Commands: stats, terms, help, quit" << std::endl;
                std::cout << std::endl;
                continue;
            }

            if (query == "stats") {
                showStats();
                continue;
            }

            if (query == "terms") {
                showTerms();
                continue;
            }

            search(query);
        }
    }
};

int main(int argc, char* argv[]) {
    std::string inverted_file = "inverted_index.bin";
    std::string forward_file = "forward_index.bin";

    if (argc > 1) {
        inverted_file = argv[1];
    }
    if (argc > 2) {
        forward_file = argv[2];
    }

    SearchEngine engine;

    std::cout << "Loading indexes..." << std::endl;
    if (!engine.loadIndex(inverted_file, forward_file)) {
        std::cerr << "Error: Cannot load indexes" << std::endl;
        std::cerr << "  Run indexer first" << std::endl;
        return 1;
    }
    std::cout << std::endl;

    engine.interactiveMode();

    return 0;
}