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
#include <cstdint>

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

class BooleanIndex {
private:
    std::map<std::string, TermRecord> inverted_index;
    std::map<int, DocRecord> forward_index;
    int doc_count;

    std::string toLower(const std::string& str) {
        std::string result = str;
        for (size_t i = 0; i < result.length(); ++i) {
            if (result[i] >= 'A' && result[i] <= 'Z') {
                result[i] = result[i] + 32;
            }
        }
        return result;
    }

    bool isValidChar(char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    }

    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::string current;

        for (size_t i = 0; i < text.length(); ++i) {
            char c = text[i];
            if (isValidChar(c)) {
                current += c;
            } else {
                if (current.length() >= 2) {
                    tokens.push_back(toLower(current));
                }
                current.clear();
            }
        }

        if (current.length() >= 2) {
            tokens.push_back(toLower(current));
        }

        return tokens;
    }

    void writeString(std::ofstream& file, const std::string& str) {
        uint32_t length = static_cast<uint32_t>(str.length());
        file.write(reinterpret_cast<const char*>(&length), sizeof(length));
        if (length > 0) {
            file.write(str.c_str(), length);
        }
    }

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

public:
    BooleanIndex() : doc_count(0) {}

    void addDocument(int doc_id, const std::string& title, 
                    const std::string& url, const std::string& content,
                    const std::string& source) {
        DocRecord doc;
        doc.doc_id = doc_id;
        doc.title = title;
        doc.url = url;
        doc.source = source;
        forward_index[doc_id] = doc;

        std::string full_text = title + " " + content;
        std::vector<std::string> tokens = tokenize(full_text);

        std::map<std::string, int> term_freq;
        for (size_t i = 0; i < tokens.size(); ++i) {
            term_freq[tokens[i]]++;
        }

        for (std::map<std::string, int>::iterator it = term_freq.begin(); 
             it != term_freq.end(); ++it) {
            if (inverted_index.find(it->first) == inverted_index.end()) {
                TermRecord record;
                record.term = it->first;
                inverted_index[it->first] = record;
            }

            bool found = false;
            for (size_t i = 0; i < inverted_index[it->first].doc_ids.size(); ++i) {
                if (inverted_index[it->first].doc_ids[i] == doc_id) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                inverted_index[it->first].doc_ids.push_back(doc_id);
                inverted_index[it->first].frequencies.push_back(it->second);
            }
        }

        doc_count++;
    }

    void buildIndex() {
        for (std::map<std::string, TermRecord>::iterator it = inverted_index.begin();
             it != inverted_index.end(); ++it) {
            std::sort(it->second.doc_ids.begin(), it->second.doc_ids.end());
        }
    }

    bool saveToBinary(const std::string& inverted_path, const std::string& forward_path) {
        std::ofstream inv_file(inverted_path.c_str(), std::ios::binary);
        if (!inv_file.is_open()) {
            std::cerr << "Error: Cannot create " << inverted_path << std::endl;
            return false;
        }

        int term_count = static_cast<int>(inverted_index.size());
        inv_file.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));

        for (std::map<std::string, TermRecord>::iterator it = inverted_index.begin();
             it != inverted_index.end(); ++it) {
            writeString(inv_file, it->first);

            int doc_count = static_cast<int>(it->second.doc_ids.size());
            inv_file.write(reinterpret_cast<const char*>(&doc_count), sizeof(doc_count));

            for (int i = 0; i < doc_count; ++i) {
                inv_file.write(reinterpret_cast<const char*>(&it->second.doc_ids[i]), sizeof(int));
                inv_file.write(reinterpret_cast<const char*>(&it->second.frequencies[i]), sizeof(int));
            }
        }
        inv_file.close();

        std::ofstream fwd_file(forward_path.c_str(), std::ios::binary);
        if (!fwd_file.is_open()) {
            std::cerr << "Error: Cannot create " << forward_path << std::endl;
            return false;
        }

        fwd_file.write(reinterpret_cast<const char*>(&this->doc_count), sizeof(int));

        for (std::map<int, DocRecord>::iterator it = forward_index.begin();
             it != forward_index.end(); ++it) {
            fwd_file.write(reinterpret_cast<const char*>(&it->second.doc_id), sizeof(int));
            writeString(fwd_file, it->second.title);
            writeString(fwd_file, it->second.url);
            writeString(fwd_file, it->second.source);
        }
        fwd_file.close();

        std::cout << "Indexes saved successfully" << std::endl;
        return true;
    }

    bool loadFromBinary(const std::string& inverted_path, const std::string& forward_path) {
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

        fwd_file.read(reinterpret_cast<char*>(&this->doc_count), sizeof(doc_count));

        for (int i = 0; i < this->doc_count; ++i) {
            DocRecord doc;
            fwd_file.read(reinterpret_cast<char*>(&doc.doc_id), sizeof(int));
            doc.title = readString(fwd_file);
            doc.url = readString(fwd_file);
            doc.source = readString(fwd_file);
            forward_index[doc.doc_id] = doc;
        }
        fwd_file.close();

        std::cout << "Indexes loaded successfully" << std::endl;
        return true;
    }

    std::set<int> searchAnd(const std::vector<std::string>& terms) {
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

    std::set<int> searchOr(const std::vector<std::string>& terms) {
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

    std::set<int> searchNot(const std::string& term) {
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

    void printStatistics() {
        int total_postings = 0;
        std::vector<std::pair<std::string, int>> term_freq;

        for (std::map<std::string, TermRecord>::iterator it = inverted_index.begin();
             it != inverted_index.end(); ++it) {
            int count = static_cast<int>(it->second.doc_ids.size());
            total_postings += count;
            term_freq.push_back(std::make_pair(it->first, count));
        }

        std::sort(term_freq.begin(), term_freq.end(),
            [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                return a.second > b.second;
            });

        std::cout << "Index Statistics:" << std::endl;
        std::cout << "Total documents: " << doc_count << std::endl;
        std::cout << "Unique terms: " << inverted_index.size() << std::endl;
        std::cout << "Total postings: " << total_postings << std::endl;
        
        if (inverted_index.size() > 0) {
            std::cout << "Average postings per term: " 
                      << std::fixed << std::setprecision(2) 
                      << (static_cast<double>(total_postings) / inverted_index.size()) 
                      << std::endl;
        }

        std::cout << "\nTop 20 terms by document frequency:" << std::endl;
        for (size_t i = 0; i < 20 && i < term_freq.size(); ++i) {
            std::cout << (i + 1) << ". " << term_freq[i].first 
                      << ": " << term_freq[i].second << " documents" << std::endl;
        }
    }

    int getDocCount() const { return doc_count; }
    int getUniqueTerms() const { return static_cast<int>(inverted_index.size()); }
};

class CorpusLoader {
public:
    static std::vector<std::map<std::string, std::string>> loadFromJSON(const std::string& filepath) {
        std::vector<std::map<std::string, std::string>> documents;
        std::ifstream file(filepath);
        
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open " << filepath << std::endl;
            return documents;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        size_t pos = 0;
        while ((pos = content.find("{", pos)) != std::string::npos) {
            size_t end = content.find("}", pos);
            if (end == std::string::npos) {
                break;
            }

            std::string doc_json = content.substr(pos, end - pos + 1);
            std::map<std::string, std::string> doc;

            doc["title"] = extractString(doc_json, "title");
            doc["url"] = extractString(doc_json, "url");
            doc["content"] = extractString(doc_json, "content");
            doc["source"] = extractString(doc_json, "source");

            if (!doc["title"].empty() || !doc["content"].empty()) {
                documents.push_back(doc);
            }

            pos = end + 1;
        }

        return documents;
    }

private:
    static std::string extractString(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) {
            return "";
        }

        pos = json.find(":", pos);
        if (pos == std::string::npos) {
            return "";
        }

        pos = json.find("\"", pos);
        if (pos == std::string::npos) {
            return "";
        }

        size_t start = pos + 1;
        size_t end = json.find("\"", start);
        if (end == std::string::npos) {
            return "";
        }

        return json.substr(start, end - start);
    }
};

int main(int argc, char* argv[]) {
    std::cout << "============================================================" << std::endl;
    std::cout << "Boolean Index - Building Index" << std::endl;
    std::cout << "============================================================" << std::endl;

    std::string corpus_file = "corpus_export.json";
    std::string inverted_file = "inverted_index.bin";
    std::string forward_file = "forward_index.bin";

    if (argc > 1) {
        corpus_file = argv[1];
    }

    std::cout << "Input file: " << corpus_file << std::endl;
    std::cout << "Inverted index: " << inverted_file << std::endl;
    std::cout << "Forward index: " << forward_file << std::endl;
    std::cout << std::endl;

    std::cout << "Loading documents..." << std::endl;
    auto load_start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::map<std::string, std::string>> docs = CorpusLoader::loadFromJSON(corpus_file);
    
    auto load_end = std::chrono::high_resolution_clock::now();
    double load_time = std::chrono::duration<double>(load_end - load_start).count();

    if (docs.empty()) {
        std::cerr << "Error: No documents found" << std::endl;
        return 1;
    }

    std::cout << "Loaded " << docs.size() << " documents" << std::endl;
    std::cout << "Load time: " << std::fixed << std::setprecision(2) << load_time << " sec" << std::endl;
    std::cout << std::endl;

    std::cout << "Building index..." << std::endl;
    auto index_start = std::chrono::high_resolution_clock::now();

    BooleanIndex index;

    for (size_t i = 0; i < docs.size(); ++i) {
        std::map<std::string, std::string>& doc = docs[i];
        index.addDocument(static_cast<int>(i), 
                         doc["title"], 
                         doc["url"], 
                         doc["content"], 
                         doc["source"]);

        if ((i + 1) % 1000 == 0) {
            std::cout << "Processed " << (i + 1) << " of " << docs.size() << " documents" << std::endl;
        }
    }

    index.buildIndex();

    auto index_end = std::chrono::high_resolution_clock::now();
    double index_time = std::chrono::duration<double>(index_end - index_start).count();

    std::cout << "Index time: " << std::fixed << std::setprecision(2) << index_time << " sec" << std::endl;
    std::cout << std::endl;

    index.printStatistics();
    std::cout << std::endl;

    std::cout << "Saving indexes..." << std::endl;
    if (!index.saveToBinary(inverted_file, forward_file)) {
        return 1;
    }
    std::cout << std::endl;

    double total_time = load_time + index_time;
    std::cout << "============================================================" << std::endl;
    std::cout << "Indexing complete!" << std::endl;
    std::cout << "Total time: " << std::fixed << std::setprecision(2) << total_time << " sec" << std::endl;
    std::cout << "Speed: " << std::fixed << std::setprecision(1) 
              << (docs.size() / total_time) << " documents/sec" << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}