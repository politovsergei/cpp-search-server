#include "../header/search_server.h"

SearchServer::SearchServer(const std::string& text) {
    for (const std::string& word : SplitIntoWords(text)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("invalid stop word"s);
        }

        if (!word.empty()) {
            stop_words_.insert(word);
        }
    }
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector <int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("invalid id"s);
    }

    if (std::find(id_base_.begin(), id_base_.end(), document_id) != id_base_.end()) {
        throw std::invalid_argument("id duplication"s);
    }

    const std::vector <std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();

    for (const std::string& word : words) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("invalid document word"s);
        }

        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_freqs_ids_[document_id][word] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    id_base_.insert(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (std::find(id_base_.begin(), id_base_.end(), document_id) != id_base_.end()) {
        std::map <std::string, std::map <int, double>>::iterator iter = word_to_document_freqs_.begin();

        while(iter != word_to_document_freqs_.end()) {
            auto& data = iter -> second;
            if (data.find(document_id) != data.end()) {
                data.erase(document_id);
            }

            ++iter;
        }

        documents_.erase(document_id);
        word_freqs_ids_.erase(document_id);
        id_base_.erase(document_id);
    }
}

std::vector <Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    for (const std::string& word : SplitIntoWords(raw_query)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("invalid query word"s);
        }
    }

    auto find_lambda = [&status](int document_id, DocumentStatus doc_status, int rating) {
        return status == doc_status;
    };

    return FindTopDocuments(raw_query, find_lambda);
}

std::vector <Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    for (const std::string& word : SplitIntoWords(raw_query)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("invalid query word"s);
        }
    }

    auto find_lambda = [](int document_id, DocumentStatus doc_status, int rating) {
        return  doc_status == DocumentStatus::ACTUAL;
    };

    return FindTopDocuments(raw_query, find_lambda);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::map <std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map <std::string, double> res;
    return word_freqs_ids_.count(document_id) ? word_freqs_ids_.at(document_id) : res;
}

std::set <int> ::const_iterator SearchServer::begin() {
    return id_base_.begin();
}

std::set <int> ::const_iterator SearchServer::end() {
    return id_base_.end();
}

std::tuple <std::vector <std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    for (const std::string& word : SplitIntoWords(raw_query)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("invalid query word"s);
        }
    }

    const Query query = ParseQuery(raw_query);
    std::vector <std::string> matched_words;

    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c) { return c >= '\0' && c < ' '; });
}

int SearchServer::ComputeAverageRating(const std::vector <int>& ratings) {
    return ratings.empty() ? 0 : std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast <int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

std::vector <std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector <std::string> words;

    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) words.push_back(word);
    }

    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;
    std::string word = text;

    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }

    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + text + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;

    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }

    return query;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}
