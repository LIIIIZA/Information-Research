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

class PorterStemmer {
private:
    std::map<std::string, int> stem_freq;
    std::map<std::string, std::string> word_to_stem;
    int total_words;
    int stemmed_words;

    bool isVowel(char c) {
        return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
    }

    bool isConsonant(char c) {
        return !isVowel(c);
    }

    int measureStem(const std::string& stem) {
        int m = 0;
        bool inVowelSequence = false;

        for (size_t i = 0; i < stem.length(); ++i) {
            if (isVowel(stem[i])) {
                if (!inVowelSequence) {
                    inVowelSequence = true;
                }
            } else {
                if (inVowelSequence) {
                    m++;
                    inVowelSequence = false;
                }
            }
        }

        return m;
    }

    bool endsWith(const std::string& word, const std::string& suffix) {
        if (word.length() < suffix.length()) {
            return false;
        }
        return word.compare(word.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    std::string removeSuffix(const std::string& word, size_t suffix_len) {
        if (word.length() <= suffix_len) {
            return word;
        }
        return word.substr(0, word.length() - suffix_len);
    }

    std::string replaceSuffix(const std::string& word, const std::string& from, const std::string& to) {
        if (endsWith(word, from)) {
            return word.substr(0, word.length() - from.length()) + to;
        }
        return word;
    }

    bool containsVowel(const std::string& word) {
        for (size_t i = 0; i < word.length(); ++i) {
            if (isVowel(word[i])) {
                return true;
            }
        }
        return false;
    }

    bool endsWithDoubleConsonant(const std::string& word) {
        if (word.length() < 2) {
            return false;
        }
        char last = word[word.length() - 1];
        char secondLast = word[word.length() - 2];
        return last == secondLast && isConsonant(last);
    }

    bool endsWithCVC(const std::string& word) {
        if (word.length() < 3) {
            return false;
        }
        size_t len = word.length();
        char c1 = word[len - 3];
        char c2 = word[len - 2];
        char c3 = word[len - 1];

        if (c3 == 'w' || c3 == 'x' || c3 == 'y') {
            return false;
        }

        return isConsonant(c1) && isVowel(c2) && isConsonant(c3);
    }

    std::string step1a(const std::string& word) {
        if (endsWith(word, "sses")) {
            return removeSuffix(word, 2);
        }
        if (endsWith(word, "ies")) {
            return removeSuffix(word, 2);
        }
        if (endsWith(word, "ss")) {
            return word;
        }
        if (endsWith(word, "s")) {
            return removeSuffix(word, 1);
        }
        return word;
    }

    std::string step1b(const std::string& word) {
        if (endsWith(word, "eed")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 0) {
                return removeSuffix(word, 1);
            }
            return word;
        }

        if (endsWith(word, "ed")) {
            if (containsVowel(word.substr(0, word.length() - 2))) {
                std::string result = removeSuffix(word, 2);
                return step1bHelper(result);
            }
            return word;
        }

        if (endsWith(word, "ing")) {
            if (containsVowel(word.substr(0, word.length() - 3))) {
                std::string result = removeSuffix(word, 3);
                return step1bHelper(result);
            }
            return word;
        }

        return word;
    }

    std::string step1bHelper(const std::string& word) {
        if (endsWith(word, "at")) {
            return word + "e";
        }
        if (endsWith(word, "bl")) {
            return word + "e";
        }
        if (endsWith(word, "iz")) {
            return word + "e";
        }
        if (endsWithDoubleConsonant(word)) {
            char last = word[word.length() - 1];
            if (last != 'l' && last != 's' && last != 'z') {
                return word.substr(0, word.length() - 1);
            }
        }
        if (measureStem(word) == 1 && endsWithCVC(word)) {
            return word + "e";
        }
        return word;
    }

    std::string step1c(const std::string& word) {
        if (word.length() > 2 && endsWith(word, "y")) {
            if (containsVowel(word.substr(0, word.length() - 1))) {
                return word.substr(0, word.length() - 1) + "i";
            }
        }
        return word;
    }

    std::string step2(const std::string& word) {
        if (endsWith(word, "ational")) {
            if (measureStem(word.substr(0, word.length() - 7)) > 0) {
                return replaceSuffix(word, "ational", "ate");
            }
        }
        if (endsWith(word, "tional")) {
            if (measureStem(word.substr(0, word.length() - 6)) > 0) {
                return replaceSuffix(word, "tional", "tion");
            }
        }
        if (endsWith(word, "enci")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 0) {
                return replaceSuffix(word, "enci", "ence");
            }
        }
        if (endsWith(word, "anci")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 0) {
                return replaceSuffix(word, "anci", "ance");
            }
        }
        if (endsWith(word, "izer")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 0) {
                return replaceSuffix(word, "izer", "ize");
            }
        }
        if (endsWith(word, "abli")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 0) {
                return replaceSuffix(word, "abli", "able");
            }
        }
        if (endsWith(word, "alli")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 0) {
                return replaceSuffix(word, "alli", "al");
            }
        }
        if (endsWith(word, "entli")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "entli", "ent");
            }
        }
        if (endsWith(word, "eli")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 0) {
                return replaceSuffix(word, "eli", "e");
            }
        }
        if (endsWith(word, "ousli")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "ousli", "ous");
            }
        }
        if (endsWith(word, "ization")) {
            if (measureStem(word.substr(0, word.length() - 7)) > 0) {
                return replaceSuffix(word, "ization", "ize");
            }
        }
        if (endsWith(word, "ation")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "ation", "ate");
            }
        }
        if (endsWith(word, "ator")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 0) {
                return replaceSuffix(word, "ator", "ate");
            }
        }
        if (endsWith(word, "alism")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "alism", "al");
            }
        }
        if (endsWith(word, "iveness")) {
            if (measureStem(word.substr(0, word.length() - 7)) > 0) {
                return replaceSuffix(word, "iveness", "ive");
            }
        }
        if (endsWith(word, "fulness")) {
            if (measureStem(word.substr(0, word.length() - 7)) > 0) {
                return replaceSuffix(word, "fulness", "ful");
            }
        }
        if (endsWith(word, "ousness")) {
            if (measureStem(word.substr(0, word.length() - 7)) > 0) {
                return replaceSuffix(word, "ousness", "ous");
            }
        }
        if (endsWith(word, "aliti")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "aliti", "al");
            }
        }
        if (endsWith(word, "iviti")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "iviti", "ive");
            }
        }
        if (endsWith(word, "biliti")) {
            if (measureStem(word.substr(0, word.length() - 6)) > 0) {
                return replaceSuffix(word, "biliti", "ble");
            }
        }
        return word;
    }

    std::string step3(const std::string& word) {
        if (endsWith(word, "icate")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "icate", "ic");
            }
        }
        if (endsWith(word, "ative")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "ative", "");
            }
        }
        if (endsWith(word, "alize")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "alize", "al");
            }
        }
        if (endsWith(word, "iciti")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 0) {
                return replaceSuffix(word, "iciti", "ic");
            }
        }
        if (endsWith(word, "ical")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 0) {
                return replaceSuffix(word, "ical", "ic");
            }
        }
        if (endsWith(word, "ful")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 0) {
                return replaceSuffix(word, "ful", "");
            }
        }
        if (endsWith(word, "ness")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 0) {
                return replaceSuffix(word, "ness", "");
            }
        }
        return word;
    }

    std::string step4(const std::string& word) {
        if (endsWith(word, "al")) {
            if (measureStem(word.substr(0, word.length() - 2)) > 1) {
                return removeSuffix(word, 2);
            }
        }
        if (endsWith(word, "ance")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 1) {
                return removeSuffix(word, 4);
            }
        }
        if (endsWith(word, "ence")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 1) {
                return removeSuffix(word, 4);
            }
        }
        if (endsWith(word, "er")) {
            if (measureStem(word.substr(0, word.length() - 2)) > 1) {
                return removeSuffix(word, 2);
            }
        }
        if (endsWith(word, "ic")) {
            if (measureStem(word.substr(0, word.length() - 2)) > 1) {
                return removeSuffix(word, 2);
            }
        }
        if (endsWith(word, "able")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 1) {
                return removeSuffix(word, 4);
            }
        }
        if (endsWith(word, "ible")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 1) {
                return removeSuffix(word, 4);
            }
        }
        if (endsWith(word, "ant")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                return removeSuffix(word, 3);
            }
        }
        if (endsWith(word, "ement")) {
            if (measureStem(word.substr(0, word.length() - 5)) > 1) {
                return removeSuffix(word, 5);
            }
        }
        if (endsWith(word, "ment")) {
            if (measureStem(word.substr(0, word.length() - 4)) > 1) {
                return removeSuffix(word, 4);
            }
        }
        if (endsWith(word, "ent")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                return removeSuffix(word, 3);
            }
        }
        if (endsWith(word, "ion")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                char before = word[word.length() - 4];
                if (before == 's' || before == 't') {
                    return removeSuffix(word, 3);
                }
            }
        }
        if (endsWith(word, "ou")) {
            if (measureStem(word.substr(0, word.length() - 2)) > 1) {
                return removeSuffix(word, 2);
            }
        }
        if (endsWith(word, "ism")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                return removeSuffix(word, 3);
            }
        }
        if (endsWith(word, "ate")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                return removeSuffix(word, 3);
            }
        }
        if (endsWith(word, "iti")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                return removeSuffix(word, 3);
            }
        }
        if (endsWith(word, "ous")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                return removeSuffix(word, 3);
            }
        }
        if (endsWith(word, "ive")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                return removeSuffix(word, 3);
            }
        }
        if (endsWith(word, "ize")) {
            if (measureStem(word.substr(0, word.length() - 3)) > 1) {
                return removeSuffix(word, 3);
            }
        }
        return word;
    }

    std::string step5a(const std::string& word) {
        if (endsWith(word, "e")) {
            if (measureStem(word.substr(0, word.length() - 1)) > 1) {
                return removeSuffix(word, 1);
            }
            if (measureStem(word.substr(0, word.length() - 1)) == 1 && !endsWithCVC(word.substr(0, word.length() - 1))) {
                return removeSuffix(word, 1);
            }
        }
        return word;
    }

    std::string step5b(const std::string& word) {
        if (measureStem(word) > 1 && endsWithDoubleConsonant(word) && endsWith(word, "l")) {
            return removeSuffix(word, 1);
        }
        return word;
    }

public:
    PorterStemmer() : total_words(0), stemmed_words(0) {}

    std::string stem(const std::string& word) {
        if (word.length() < 3) {
            return word;
        }

        std::string result = word;
        result = step1a(result);
        result = step1b(result);
        result = step1c(result);
        result = step2(result);
        result = step3(result);
        result = step4(result);
        result = step5a(result);
        result = step5b(result);

        return result;
    }

    void processFile(const std::string& input_path, const std::string& output_path) {
        std::ifstream infile(input_path);
        std::ofstream outfile(output_path);

        if (!infile.is_open()) {
            std::cerr << "Error: Cannot open input file " << input_path << std::endl;
            return;
        }

        if (!outfile.is_open()) {
            std::cerr << "Error: Cannot create output file " << output_path << std::endl;
            return;
        }

        std::string word;
        while (infile >> word) {
            total_words++;
            std::string stemmed = stem(word);
            stem_freq[stemmed]++;
            word_to_stem[word] = stemmed;
            
            if (stemmed != word) {
                stemmed_words++;
            }

            outfile << stemmed << "\n";
        }

        infile.close();
        outfile.close();
    }

    void printStatistics() {
        std::cout << "Stemming Statistics:" << std::endl;
        std::cout << "Total words processed: " << total_words << std::endl;
        std::cout << "Words stemmed: " << stemmed_words << std::endl;
        std::cout << "Unique stems: " << stem_freq.size() << std::endl;
        
        if (total_words > 0) {
            double reduction = (1.0 - static_cast<double>(stem_freq.size()) / total_words) * 100;
            std::cout << "Vocabulary reduction: " << std::fixed << std::setprecision(2) 
                      << reduction << "%" << std::endl;
        }

        std::cout << "\nTop 20 stems by frequency:" << std::endl;
        
        std::vector<std::pair<std::string, int>> sorted_stems(stem_freq.begin(), stem_freq.end());
        std::sort(sorted_stems.begin(), sorted_stems.end(), 
            [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                return a.second > b.second;
            });

        for (size_t i = 0; i < 20 && i < sorted_stems.size(); ++i) {
            std::cout << (i + 1) << ". " << sorted_stems[i].first 
                      << ": " << sorted_stems[i].second << std::endl;
        }

        std::cout << "\nSample word-to-stem mappings:" << std::endl;
        int count = 0;
        for (const auto& pair : word_to_stem) {
            if (pair.first != pair.second && count < 15) {
                std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
                count++;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    auto start = std::chrono::high_resolution_clock::now();

    PorterStemmer stemmer;

    if (argc > 2) {
        std::string input_file = argv[1];
        std::string output_file = argv[2];
        std::cout << "Processing: " << input_file << " -> " << output_file << std::endl;
        stemmer.processFile(input_file, output_file);
    } else {
        std::cout << "Usage: stemmer <input_file> <output_file>" << std::endl;
        return 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    stemmer.printStatistics();

    std::cout << "\nExecution time: " << duration.count() << " ms" << std::endl;

    return 0;
}