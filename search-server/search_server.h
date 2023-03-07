#pragma once

#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <numeric>
#include <stdexcept>

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double ACCURACY = 1e-6;

class SearchServer {
    public:
        template <typename StringCollection>
        explicit SearchServer(const StringCollection& stop_words);
        explicit SearchServer(const std::string& text);

        void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector <int>& ratings);

        template <typename KeyMapper>
        std::vector <Document> FindTopDocuments(const std::string& raw_query, KeyMapper k_mapper) const;
        std::vector <Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
        std::vector <Document> FindTopDocuments(const std::string raw_query) const;

        int GetDocumentCount() const;
        int GetDocumentId(const int index) const;

        std::tuple <std::vector <std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    private:
        struct DocumentData {
            int rating = 0;
            DocumentStatus status;
        };

        struct QueryWord {
            std::string data;
            bool is_minus = false;
            bool is_stop = false;
        };

        struct Query {
            std::set <std::string> plus_words;
            std::set <std::string> minus_words;
        };

        /**--- DATA ---**/
        std::set <std::string> stop_words_;
        std::map <std::string, std::map <int, double>> word_to_document_freqs_;
        std::map <int, DocumentData> documents_;
        std::vector <int> id_base_;
        /**------------**/

        bool IsStopWord(const std::string& word) const;

        static bool IsValidWord(const std::string& word);
        static int ComputeAverageRating(const std::vector <int>& ratings);

        std::vector <std::string> SplitIntoWordsNoStop(const std::string& text) const;
        double ComputeWordInverseDocumentFreq(const std::string& word) const;

        QueryWord ParseQueryWord(std::string text) const;
        Query ParseQuery(const std::string& text) const;

        template <typename KeyMapper>
        std::vector <Document> FindAllDocuments(const Query& query, KeyMapper& k_mapper) const;
};

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words) {
    for (const std::string& word : std::set <std::string> (stop_words.begin(), stop_words.end())) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("invalid stop word"s);
        }

        if (!word.empty()) {
            stop_words_.insert(word);
        }
    }

}

template <typename KeyMapper>
std::vector <Document> SearchServer::FindTopDocuments(const std::string& raw_query, KeyMapper k_mapper) const {
    for (const std::string& word : SplitIntoWords(raw_query)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("invalid query word"s);
        }
    }

    const Query query = ParseQuery(raw_query);
    std::vector <Document> result = FindAllDocuments(query, k_mapper);

    auto lambda_sort = [](const Document& lhs, const Document& rhs) {
        return std::abs(lhs.relevance - rhs.relevance) < ACCURACY ? lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
    };

    std::sort(result.begin(), result.end(), lambda_sort);

    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return result;
}

template <typename KeyMapper>
std::vector <Document> SearchServer::FindAllDocuments(const Query& query, KeyMapper& k_mapper) const {
    std::map <int, double> document_to_relevance;
    std::vector <Document> matched_documents;

    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            if (k_mapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }

    return matched_documents;
}
