#include "search_server.h"

SearchServer::SearchServer(const string& text) {
    for (const string& word : SplitIntoWords(text)) {
        if (!SearchServer::IsValidWord(word))
            throw invalid_argument("invalid stop word"s);
        if (!word.empty())
            stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector <int>& ratings) {
    if (document_id < 0)
        throw invalid_argument("invalid id"s);
    if (find(id_base_.begin(), id_base_.end(), document_id) != id_base_.end())
        throw invalid_argument("id duplication"s);

    const vector <string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();

    for (const string& word : words) {
        if (!SearchServer::IsValidWord(word))
            throw invalid_argument("invalid document word"s);
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    id_base_.push_back(document_id);
}

vector <Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    for (const string& word : SplitIntoWords(raw_query))
        if (!SearchServer::IsValidWord(word)) throw invalid_argument("invalid query word"s);

    return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus doc_status, int rating) { return status == doc_status; });
}

vector <Document> SearchServer::FindTopDocuments(const string raw_query) const {
    for (const string& word : SplitIntoWords(raw_query))
        if (!SearchServer::IsValidWord(word)) throw invalid_argument("invalid query word"s);

    return FindTopDocuments(raw_query, [](int document_id, DocumentStatus doc_status, int rating) { return  doc_status == DocumentStatus::ACTUAL; });
}

int SearchServer::GetDocumentCount() const { return documents_.size(); }

int SearchServer::GetDocumentId(const int index) const {
    if (index > static_cast <int> (id_base_.size() - 1) || index < 0)
        throw out_of_range("out of range");
    return id_base_.at(index);
}

tuple <vector <string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    for (const string& word : SplitIntoWords(raw_query))
        if (!SearchServer::IsValidWord(word)) throw invalid_argument("invalid query word"s);

    const Query query = ParseQuery(raw_query);
    vector <string> matched_words;

    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) continue;
        if (word_to_document_freqs_.at(word).count(document_id)) matched_words.push_back(word);
    }

    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) continue;
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }

    tuple <vector <string>, DocumentStatus> result = {matched_words, documents_.at(document_id).status};
    return result;
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string& word) {
    if ((word.length() == 1 && word[0] == '-') || word[word.length() - 1] == '-')
        return false;
    string buffer = "11"s;
    for (const char& ch : word) {
        buffer += ch;
        buffer.erase(0, 1);
        if (buffer == "--") return false;
    }

    return none_of(word.begin(), word.end(), [](char c) { return c >= '\0' && c < ' '; });
}

vector <string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector <string> words;

    for (const string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) words.push_back(word);
    }

    return words;
}

int SearchServer::ComputeAverageRating(const vector <int>& ratings) {
    if (ratings.empty()) return 0;
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast <int>(ratings.size()); // FIX_2
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query query;
    for (const string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) query.minus_words.insert(query_word.data);
            else query.plus_words.insert(query_word.data);
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
