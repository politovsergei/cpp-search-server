#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double ACCURACY = 1e-6;

enum class DocumentStatus { ACTUAL, IRRELEVANT, BANNED, REMOVED };

struct Document {
    Document(const int& id_, const double& relevance_, const int rating_)
        : id(id_), relevance(relevance_), rating(rating_) {}

    Document() {
        id = 0;
        relevance = 0;
        rating = 0;
    }

    int id;
    double relevance;
    int rating;
};

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector <string> SplitIntoWords(const string& text) {
    vector <string> words;
    string word;

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

class SearchServer {
    public:
        template <typename StringCollection>
        explicit SearchServer(const StringCollection& stop_words) {
            for (const string& word : set <string> (stop_words.begin(), stop_words.end())) {
                if (!IsValidWord(word)) {
                    throw invalid_argument("invalid stop word"s);
                }
                if (!word.empty())
                    stop_words_.insert(word);
            }
        }

        explicit SearchServer(const string& text) {
            SearchServer(SplitIntoWords(text));
        }

        void AddDocument(int document_id, const string& document, DocumentStatus status, const vector <int>& ratings) {
            if (document_id < 0) {
                throw invalid_argument("invalid id"s);
            }

            if (find(id_base_.begin(), id_base_.end(), document_id) != id_base_.end()) {
                throw invalid_argument("id duplication"s);
            }

            if (documents_.count(document_id)) {
                throw invalid_argument("id duplication"s);
            }

            const vector <string> words = SplitIntoWordsNoStop(document);
            const double inv_word_count = 1.0 / words.size();

            for (const string& word : words) {
                if (!IsValidWord(word)) {
                    throw invalid_argument("invalid document word"s);
                }
                word_to_document_freqs_[word][document_id] += inv_word_count;
            }

            documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
            id_base_.push_back(document_id);
        }

        template <typename KeyMapper>
        vector <Document> FindTopDocuments(const string& raw_query, KeyMapper k_mapper) const {
            const Query query = ParseQuery(raw_query);
            vector <Document> result = FindAllDocuments(query, k_mapper);

            sort(result.begin(), result.end(),
                 [](const Document& lhs, const Document& rhs) {
                     if (abs(lhs.relevance - rhs.relevance) < ACCURACY) {
                        return lhs.rating > rhs.rating;
                     }
                     else {
                        return lhs.relevance > rhs.relevance;
                     }
                 });

            if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
                result.resize(MAX_RESULT_DOCUMENT_COUNT);
            }

            return result;
        }

        vector <Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
            return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus doc_status, int rating) {
                                        return status == doc_status;
                                    });
        }

        vector <Document> FindTopDocuments(const string raw_query) const {
            return FindTopDocuments(raw_query, [](int document_id, DocumentStatus doc_status, int rating) {
                                        return  doc_status == DocumentStatus::ACTUAL;
                                    });
        }

        int GetDocumentCount() const { return documents_.size(); }

        int GetDocumentId(const int index) const {
            if (index > static_cast <int> (id_base_.size() - 1) ||
                index < 0) {

                throw out_of_range("out of range");
            }

            return id_base_.at(index);
        }

        tuple <vector <string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
            const Query query = ParseQuery(raw_query);
            vector <string> matched_words;

            for (const string& word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }

            for (const string& word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.clear();
                    break;
                }
            }
            tuple <vector <string>, DocumentStatus> result = {matched_words, documents_.at(document_id).status};
            return result;
        }

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

        /**DATA**/
        set <string> stop_words_;
        map <string, map <int, double>> word_to_document_freqs_;
        map <int, DocumentData> documents_;
        vector <int> id_base_;

        bool IsStopWord(const string& word) const {
            return stop_words_.count(word) > 0;
        }

        static bool IsCorrectMinusWord(const string& word) {
            if ((word.length() == 1 && word[0] == '-') ||
                word[word.length() - 1] == '-') {

                return false;
            }

            string buffer;
            for (const char& ch : word) {
                buffer += ch;

                if (buffer.length() > static_cast <size_t> (2)) {
                    buffer.erase(0, 1);
                }

                if (buffer == "--"s) {
                    return false;
                }
            }

            return true;
        }

        static bool IsValidWord(const string& word) {
            return none_of(word.begin(), word.end(), [](char c) { return c >= '\0' && c < ' '; });
        }

        vector <string> SplitIntoWordsNoStop(const string& text) const {
            vector <string> words;

            for (const string& word : SplitIntoWords(text)) {
                if (!IsStopWord(word)) {
                    words.push_back(word);
                }
            }

            return words;
        }

        static int ComputeAverageRating(const vector <int>& ratings) {
            if (ratings.empty()) {
                return 0;
            }

            return accumulate(ratings.begin(), ratings.end(), 0) / static_cast <int>(ratings.size());
        }

        QueryWord ParseQueryWord(string text) const {
            bool is_minus = false;
            if (text[0] == '-') {
                is_minus = true;
                text = text.substr(1);

                if (IsCorrectMinusWord(text)) {
                    throw invalid_argument("invalid minus word"s);
                }
            }
            return {text, is_minus, IsStopWord(text)};
        }

        Query ParseQuery(const string& text) const {
            Query query;

            for (const string& word : SplitIntoWords(text)) {
                if (!IsValidWord(word)) {
                    throw invalid_argument("invalid query word"s);
                }

                const QueryWord query_word = ParseQueryWord(word);
                if (!query_word.is_stop) {
                    if (query_word.is_minus) query.minus_words.insert(query_word.data);
                    else query.plus_words.insert(query_word.data);
                }
            }
            return query;
        }

        double ComputeWordInverseDocumentFreq(const string& word) const {
            return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
        }

        template <typename KeyMapper>
        vector <Document> FindAllDocuments(const Query& query, KeyMapper& k_mapper) const {
            map <int, double> document_to_relevance;

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

            vector <Document> matched_documents;

            for (const auto [document_id, relevance] : document_to_relevance) {
                matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
            }

            return matched_documents;
        }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
            PrintDocument(document);
        }
    } catch (const exception& er) {
        cout << er.what() << endl;
    }

    cout << "BANNED:"s << endl;
    try {
        for (const Document& document :
             search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
            PrintDocument(document);
        }
    } catch (const exception& er) {
        cout << er.what() << endl;
    }


    cout << "Even ids:"s << endl;
    try {
        for (const Document &document :
         search_server.FindTopDocuments("пушистый ухоженный кот"s,
                [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0;}))
                    PrintDocument(document);
    } catch (const exception& er) {
        cout << er.what() << endl;
    }

    return 0;
}