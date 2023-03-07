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

/** � �� ���� ������ �������, �� ������� ������� ����-����� ���� �� ������������ ����� using namespace.
    ������� �� ��� �������� ���, � ����� ����������� ������������ ��� ���� ������, �� ������ � ���� �����.**/

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double ACCURACY = 1e-6;

class SearchServer {
    public:
        template <typename StringCollection>
        explicit SearchServer(const StringCollection& stop_words);
        explicit SearchServer(const string& text);

        void AddDocument(int document_id, const string& document, DocumentStatus status, const vector <int>& ratings);

        template <typename KeyMapper>
        vector <Document> FindTopDocuments(const string& raw_query, KeyMapper k_mapper) const;
        vector <Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const;
        vector <Document> FindTopDocuments(const string raw_query) const;

        int GetDocumentCount() const;
        int GetDocumentId(const int index) const;

        tuple <vector <string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;

    private:
        struct DocumentData {
            int rating = 0;
            DocumentStatus status;
        };

        struct QueryWord {
            string data;
            bool is_minus = false;
            bool is_stop = false;
        };

        struct Query {
            set <string> plus_words;
            set <string> minus_words;
        };

        /**--- DATA ---**/
        set <string> stop_words_;
        map <string, map <int, double>> word_to_document_freqs_;
        map <int, DocumentData> documents_;
        vector <int> id_base_;
        /**------------**/

        bool IsStopWord(const string& word) const;

        static bool IsValidWord(const string& word);
        static int ComputeAverageRating(const vector <int>& ratings);

        vector <string> SplitIntoWordsNoStop(const string& text) const;
        double ComputeWordInverseDocumentFreq(const string& word) const;

        QueryWord ParseQueryWord(string text) const;
        Query ParseQuery(const string& text) const;

        template <typename KeyMapper>
        vector <Document> FindAllDocuments(const Query& query, KeyMapper& k_mapper) const;
};

template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& stop_words) {
    for (const string& word : set <string> (stop_words.begin(), stop_words.end())) {
        if (!SearchServer::IsValidWord(word)) {
            throw invalid_argument("invalid stop word"s);
        }

        if (!word.empty()) {
            stop_words_.insert(word);
        }
    }

}

template <typename KeyMapper>
vector <Document> SearchServer::FindTopDocuments(const string& raw_query, KeyMapper k_mapper) const {
    for (const string& word : SplitIntoWords(raw_query)) {
        if (!SearchServer::IsValidWord(word)) {
            throw invalid_argument("invalid query word"s);
        }
    }

    const Query query = ParseQuery(raw_query);
    vector <Document> result = FindAllDocuments(query, k_mapper);

    auto lambda_sort = [](const Document& lhs, const Document& rhs) {
        return abs(lhs.relevance - rhs.relevance) < ACCURACY ? lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
    };

    sort(result.begin(), result.end(), lambda_sort);

    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return result;
}

template <typename KeyMapper>
vector <Document> SearchServer::FindAllDocuments(const Query& query, KeyMapper& k_mapper) const {
    map <int, double> document_to_relevance;
    vector <Document> matched_documents;

    for (const string& word : query.plus_words) {
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

    for (const string& word : query.minus_words) {
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
