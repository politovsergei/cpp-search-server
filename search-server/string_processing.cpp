#include "string_processing.h"

std::vector <std::string> SplitIntoWords(const std::string& text) {
    std::vector <std::string> words;
    std::string word;

    for (size_t index = 0; index < text.length(); ++index) {
        if (text[index] == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
            continue;
        } else word += text[index];
    }

    if (!word.empty()) words.push_back(word);

    return words;
}
