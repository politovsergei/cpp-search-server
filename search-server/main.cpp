#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double ACCURACY = 1e-6;

enum class DocumentStatus { ACTUAL, IRRELEVANT, BANNED, REMOVED };

struct Document {
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
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
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else word += c;
    }

    if (!word.empty()) words.push_back(word);

    return words;
}

class SearchServer {
    public:
        void SetStopWords(const string& text) {
            for (const string& word : SplitIntoWords(text)) {
                stop_words_.insert(word);
            }
        }

        void AddDocument(int document_id, const string& document, DocumentStatus status, const vector <int>& ratings) {
            const vector <string> words = SplitIntoWordsNoStop(document);
            const double inv_word_count = 1.0 / words.size();

            for (const string& word : words) word_to_document_freqs_[word][document_id] += inv_word_count;
            documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        }

        template <typename KeyMapper>
        vector <Document> FindTopDocuments(const string& raw_query, KeyMapper k_mapper) const {
            const Query query = ParseQuery(raw_query);
            auto matched_documents = FindAllDocuments(query, k_mapper);

            sort(matched_documents.begin(), matched_documents.end(),
                 [](const Document& lhs, const Document& rhs) {
                     if (abs(lhs.relevance - rhs.relevance) < ACCURACY) return lhs.rating > rhs.rating;
                     else return lhs.relevance > rhs.relevance;
                 });

            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);

            return matched_documents;
        }

        vector <Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
            return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus doc_status, int rating) { return status == doc_status; });
        }

        int GetDocumentCount() const { return documents_.size(); }

        tuple <vector <string>, DocumentStatus> MatchDocument(const string& raw_query,int document_id) const {
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

            return {matched_words, documents_.at(document_id).status};
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

        bool IsStopWord(const string& word) const { return stop_words_.count(word) > 0; }

        vector <string> SplitIntoWordsNoStop(const string& text) const {
            vector <string> words;

            for (const string& word : SplitIntoWords(text)) {
                if (!IsStopWord(word)) words.push_back(word);
            }

            return words;
        }

        static int ComputeAverageRating(const vector <int>& ratings) {
            if (ratings.empty()) return 0;
            return accumulate(ratings.begin(), ratings.end(), 0) / static_cast <int>(ratings.size()); // FIX_2
        }

        QueryWord ParseQueryWord(string text) const {
            bool is_minus = false;
            if (text[0] == '-') {
                is_minus = true;
                text = text.substr(1);
            }
            return {text, is_minus, IsStopWord(text)};
        }

        Query ParseQuery(const string& text) const {
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

        double ComputeWordInverseDocumentFreq(const string& word) const {
            return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
        }

        template <typename KeyMapper>
        vector <Document> FindAllDocuments(const Query& query, KeyMapper& k_mapper) const {
            map <int, double> document_to_relevance;
            for (const string& word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) continue;

                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    if (k_mapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                        document_to_relevance[document_id] += term_freq * inverse_document_freq;
                    }
                }
            }

            for (const string& word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) continue;
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

        // SERVER DATA
        set <string> stop_words_;
        map <string, map <int, double>> word_to_document_freqs_;
        map <int, DocumentData> documents_;
};

/** ---------------- ASSERTIONS IMPLIMENTATION ------------------ **/

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func) RunTestImpl(func, #func)

template <typename Func>
void RunTestImpl(const Func& test_func, const string& test_func_name) {
    test_func();
    cerr << test_func_name << " OK"s << endl;
}

string getFileNameByFilePath(string file_path) {
    string file_name;
    reverse(file_path.begin(), file_path.end());
    for(const char& ch : file_path) {
        if (ch == '\\' || ch == '/') break;
        file_name += ch;
    }
    reverse(file_name.begin(), file_name.end());
    return file_name;
}

template <typename T, typename U>
void AssertEqualImpl(
        const T& t,
        const U& u,
        const string& t_str,
        const string& u_str,
        const string& file,
        const string& func,
        unsigned line,
        const string& hint) {

    if (t != u) {
        cerr << boolalpha;
        cerr << getFileNameByFilePath(file) << "("s << line << "): "s << func << ": "s << "ASSERT_EQUAL("s
             << t_str << ", "s << u_str << ") failed: "s << "."s;

        if (!hint.empty()) cerr << " Hint: "s << hint;
        cerr << endl;
        abort();
    }
}

void AssertImpl(
        bool value,
        const string& expr_str,
        const string& file,
        const string& func,
        unsigned line,
        const string& hint) {

    if (!value) {
        cerr << getFileNameByFilePath(file) << "("s << line << "): "s << func << ": "s << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) cerr << " Hint: "s << hint;
        cerr << endl;
        abort();
    }
}

/** -------- Начало модульных тестов поисковой системы ---------- **/

// Тестовые данные
const vector <string> document_raws = {
        "white cat and fashionable collar"s,
        "fluffy cat fluffy tail"s,
        "well-groomed dog expressive eyes"s,
        "well-groomed evgeniy starling"s,
        "a book on red table"s,
        "the human in the blue sky"s,
        "windows is a good operation system"s,
        "system of a down make a bad music"s,
        "australian winter in a jule"s,
        "fireball in humane face"s,
        "doomed airplane on the color"s,
        "airplane in the red space"s};

const vector <string> status_line = { "ACTUAL"s, "IRRELEVANT"s, "BANNED"s, "REMOVED"s };
const string test_query = "well-groomed white cat with red and blue music face jump in windows at airplane winter face"s;
const string stop_words = "and in on the at of a is"s;
const vector <vector <int>> document_ratings = {{8, -3}, {7, 2, 7}, {5, -12, 2, 1}, {2, 8, -1, 2}, {10, 7, 3 , -5},
                                                {1, 1, 9}, {10, -3}, {3, 10, -5}, {-1, -1, 3}, {2, 3, -4}, {1, 2}, {1, 2 ,3}};
SearchServer server;


// Проверка колличества документов
void Test_AddDocument() {
    set <int> id_;

    for (int status = 0; status < 4; ++status)
        for (const Document& document : server.FindTopDocuments(test_query, static_cast<DocumentStatus>(status)))
            id_.insert(document.id);

    ASSERT_HINT(id_.size() == static_cast <size_t> (server.GetDocumentCount()),
                "Колличество найденых документов не соответсвует колличеству добавленных"s);
}

// Поиск доументов по стоп-словам
void Test_StopWords() {
    for (const string& word : SplitIntoWords(stop_words))
        ASSERT_HINT(server.FindTopDocuments(word, [](int document_id, DocumentStatus status, int rating) { return true; }).empty(),
                    "Найденные документы по стоп-слову: "s + word);
}

// Проверка исключений документов через минус-слова
void Test_MinusWords() {
    const string pt_1 = "5110589"s, pt_2 = "5115896"s, pt_3 = "505896"s, pt_4 = "5110586"s, pt_5 = "5110896"s, pt_6 = "5110596"s;

    vector <string> query_words = SplitIntoWords(test_query);
    const vector <string> check_data =
        {pt_1, pt_2, pt_2, pt_1, pt_3, pt_1, pt_5, pt_1, pt_4, pt_1, pt_1, pt_1, pt_1, pt_3, pt_6, pt_4};

    string query_buffer, check_buffer;
    vector <Document> documents;

    for (size_t index = 0; index < query_words.size(); ++index) {
        for (size_t word_index = 0; word_index < query_words.size(); ++word_index)
            word_index == index ?
                query_buffer += "-"s + query_words.at(word_index) + " "s :
                query_buffer += query_words.at(word_index) + " "s;

        documents = server.FindTopDocuments(query_buffer, [](int document_id, DocumentStatus status, int rating) { return true; });

        check_buffer += to_string(documents.size());
        for (const Document& document : documents) check_buffer += to_string(document.id);

        ASSERT_HINT(check_data.at(index) == check_buffer, "Из выдачи не исключены документы по слову: "s + query_words.at(index));

        query_buffer.clear();
        check_buffer.clear();
    }
}

// Проверка колличесва и индексов документов на основе поискового запроса
void Test_MatchingDocuments() {
    const vector <vector <size_t>> check_base = {{1, 1}, {1, 0}, {0}, {0}, {2, 4, 11}, {1, 5}, {1, 7}, {0}, {0}, {0}, {1, 11}, {1, 4}};
    vector <Document> documents;

    for (size_t index = 0; index < 12; ++index) {
        documents = server.FindTopDocuments("-"s + document_raws.at(index),
            [](int document_id, DocumentStatus status, int rating) { return true; });

        ASSERT_EQUAL_HINT(check_base.at(index).at(0), documents.size(),
            "Строка: "s + document_raws.at(index) + ". Haйдено: "s + to_string(documents.size()) +
            ". Конроль: " + to_string(check_base.at(index).at(0)));

        for (const Document& document : documents) {
            bool flag = find(document_raws.at(index).begin() + 1, document_raws.at(index).end(), document.id) == document_raws.at(index).end();
            ASSERT_HINT(flag, "Строка: "s + document_raws.at(index) + "Документ "s + to_string(document.id) + " не включен в выдачу"s);
        }
    }
}

// Сортировка по релевантности
void Test_RelevanceSort() {
    constexpr double EPS = 1e-6;
    vector <Document> result;
    for (int index = 0; index < 4; ++index) {
        result = server.FindTopDocuments(test_query, static_cast <DocumentStatus> (index));
        ASSERT_HINT(result.size() == 3, "Ожидается 3 документа в выдаче по статусу "s + status_line.at(index));
        for (int id_index = 0; id_index < 2; ++id_index) {
            bool eps_flag = result.at(id_index).relevance - result.at(id_index + 1).relevance < EPS;
            bool inq_flag = result.at(id_index).relevance > result.at(id_index + 1).relevance;
            ASSERT_HINT(inq_flag || eps_flag, "Некорректная сортировка выдачи по статусу "s + status_line.at(index));
        }
    }
}

// Проверка рассчёта рейтинга
void Test_RatingCompute() {
    const map <int, int> id_and_rating_pattern =
        {{0, 2}, {1, 5}, {2, -1}, {3, 2}, {4, 3}, {5, 3},
         {6, 3}, {7, 2}, {8, 0},{9, 0}, {10, 1}, {11, 2}};

    for (int num_of_status = 0; num_of_status < 4; ++num_of_status)
        for (const Document& document : server.FindTopDocuments(test_query, static_cast<DocumentStatus>(num_of_status)))
            ASSERT_EQUAL_HINT(id_and_rating_pattern.at(document.id), document.rating,
                              "Некорретный рассчёт рейтинга. Контроль: "s + to_string(id_and_rating_pattern.at(document.id)) +
                              " Результат: "s + to_string(document.rating));
}

// Сортирока по предикат-функции - ищем только документы с чётными индексами
void Test_PredicateSort() {
    vector <Document> result, buffer;
    set <int> id_set;

    for (int index = 0; index < 4; ++index) {
        auto predicateFunction = [index](int document_id, DocumentStatus status, int rating) {
            return document_id % 2 == 0 && status == static_cast <DocumentStatus> (index);
        };

        buffer = server.FindTopDocuments(test_query, predicateFunction);
        result.insert(result.end(), buffer.begin(), buffer.end());
        buffer.clear();
    }

    for (const Document& document : result) id_set.insert(document.id);
    for (const int& id : id_set)
        ASSERT_HINT(id % 2 == 0, "В выдачу попадает документ с нечетным индексом"s);
    ASSERT_EQUAL_HINT(id_set.size(), static_cast <size_t> (6), "Неверное колличество документов в выдаче. Ожидается 6"s);
}

// Проверка попадания доументов только с нужным статусом
void Test_SearchByStatus() {
    const map <DocumentStatus, set <int>> status_and_id =
        {{DocumentStatus::ACTUAL, {0, 1, 2}}, {DocumentStatus::IRRELEVANT, {3, 4, 5}},
        {DocumentStatus::BANNED, {6, 7, 8}}, {DocumentStatus::REMOVED, {9, 10, 11}}};

    for (int status = 0; status < 4; ++status) {
        DocumentStatus document_status = static_cast<DocumentStatus>(status);
        for (const Document& document : server.FindTopDocuments(test_query, document_status))
             ASSERT(status_and_id.at(document_status).find(document.id) != status_and_id.at(document_status).end());
    }
}

// Тест рассчёт релеватности
void Test_RelevanceCompute() {
    constexpr double EPS = 1e-6;
    const double R_1 = 2.31162, R_2 = 0.44794, R_3 = 0.597253, R_4 = 2.253858, R_5 = 2.484907, R_6 = 2.022809, R_7 = 2.346277, R_8 = 0.358352;
    const vector <map <int, double>> id_relevance_PATTERN =
    {{{0, R_1}, {1, R_2}}, {{0, R_2}, {1, R_1}}, {{2, R_1}, {3, R_3}}, {{2, R_2}, {3, R_4}}, {{4, R_4}, {11, R_3}},
    {{5, R_5}}, {{6, R_1}, {7, R_8}}, {{7, R_7}, {6, R_2}}, {{8, R_5}}, {{9, R_5}}, {{10, R_4}, {11, R_3}}, {{4, R_3}, {10, R_3}, {11, R_6}}};

    vector <Document> documents, buffer;

    for (size_t index = 0; index < document_raws.size(); ++index) {

        for (int statud_index = 0; statud_index < 4; ++statud_index) {
            buffer = server.FindTopDocuments(document_raws.at(index), static_cast <DocumentStatus> (statud_index));
            documents.insert(documents.end(), buffer.begin(), buffer.end());
        }

        ASSERT_HINT(documents.size() == id_relevance_PATTERN.at(index).size(), "Неверное колличество документов в выдаче"s);
        for (const Document& document : documents)
            ASSERT_HINT(abs(id_relevance_PATTERN.at(index).at(document.id) - document.relevance) < EPS,
                        "Неверный рассчёт релевантности. Контроль: "s  +
                        to_string(id_relevance_PATTERN.at(index).at(document.id)) +
                        " Результат: "s + to_string(document.relevance));
        documents.clear();
    }
}

// Основная процедура тестирования
void TestSearchServer() {
    server.SetStopWords(stop_words);

    for (int index = 0; index < 12; ++index)
        server.AddDocument(index, document_raws.at(index),
            static_cast <DocumentStatus>(index / 3), document_ratings.at(index));

    RUN_TEST(Test_AddDocument);
    RUN_TEST(Test_StopWords);
    RUN_TEST(Test_MinusWords);
    RUN_TEST(Test_MatchingDocuments);
    RUN_TEST(Test_RelevanceSort);
    RUN_TEST(Test_RatingCompute);
    RUN_TEST(Test_PredicateSort);
    RUN_TEST(Test_SearchByStatus);
    RUN_TEST(Test_RelevanceCompute);
}

/** --------- Окончание модульных тестов поисковой системы ----------- **/

int main() {
    TestSearchServer();
    cout << "Search server testing finished"s << endl;

    return 0;
}