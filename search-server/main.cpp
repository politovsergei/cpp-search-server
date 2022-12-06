#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

struct Document {
    int id;
    double relevance;
};


class SearchServer {
    public:
        void SetStopWords(const string& text) {
            for (const string& word : SplitIntoWords(text)) { stop_words_.insert(word); }
        }

        /**Добавляем документы по принципу инверсии индекса, TF вычисляется тут же.
            Дистрибутивное свойство деления тут пригодится.**/
        void AddDocument(const int document_id, const string& document) {
            const vector <string> query_words = SplitIntoWordsNoStop(document);
            double tf_ratio = 1.0 / static_cast <double> (query_words.size());

            for (const string& word : query_words) {
                    words_id_tf_[word][document_id] += tf_ratio;
            }

            ++doc_count_;
        }

        /**Формируем выдачу, согласно релевантности, сверху-вниз.
            Колличество выдачи устанавливается константой MAX_RESULT_DOCUMENT_COUNT**/
        vector <Document> FindTopDocuments(const string& raw_query) const {
            vector <Document> docs = FindAllDocuments(ParseQuery(raw_query));

            sort(docs.begin(), docs.end(), [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance; });

            if (docs.size() > MAX_RESULT_DOCUMENT_COUNT)
                docs.resize(MAX_RESULT_DOCUMENT_COUNT);
            return docs;
        }

    private:
        /**Без этой структуры поиск эффективным не сделать.**/
        struct Query {
            set <string> p_words;
            set <string> m_words;
        };

        /**Структура из конченого слова запроса и флага,
            указывающего на "плюс" или "минус" -слово.**/
        struct Word {
            string word;
            bool m_flag;
        };

        vector <string> SplitIntoWords(const string& text) const {
            vector <string> words;
            string word;
            for (const char ch : text) {
                if (ch == 32 && !word.empty()) {
                    words.push_back(word);
                    word.clear();
                } else word += ch;
            }
            if (!word.empty()) words.push_back(word);

            return words;
        }

        vector <string> SplitIntoWordsNoStop(const string& text) const {
            if (stop_words_.size() == 0) return SplitIntoWords(text);

            vector <string> words;

            for (const string& word : SplitIntoWords(text)) {
                if (stop_words_.count(word) == 0) { words.push_back(word); }
            }

            return words;
        }

        /**Пасринг слова в структуру.**/
        Word ParseQueryWord(const string& word) const {
            if (word[0] == '-') return {word.substr(1), true};
            return {word, false};
        }

        /**Теперь все документы парсятся в струтуру.
            Плюс-слова и минус-слова отдельно по уникальным спискам.**/
        Query ParseQuery(const string& text) const {
            Query query;
            for (const string& word : SplitIntoWordsNoStop(text)) {
                Word word_struct = ParseQueryWord(word);
                word_struct.m_flag ? query.m_words.insert(word_struct.word) : query.p_words.insert(word_struct.word);
            }

            return query;
        }

        /**Метод, который вычисляет IDF.
            Так мы разгрузим логику метода FindAllDocuments.**/
        double IDFCompute(const int& inc_docs_num) const {
            return log(static_cast <double> (doc_count_) / static_cast <double> (inc_docs_num));
        }

        /**Если плюс слова пусты, то вернём пустоту.
            Но вот если запрос не пуст, то пройдёмся по всем плюсам,
            попутно проверяя, нет ли их в минус-словах, и посчитаем релевантность.
            IDF вычисляется раз за проход внешнего цикла, чтобы не делать это много-много раз.
            И под финал всё сдобрим отсечением документов с минусами.
            Всё перепарсим в вектор структур и на выход.
            **/
        vector <Document> FindAllDocuments(const Query& query) const {
            if (query.p_words.size() == 0) return vector <Document>();
            vector <Document> matched_docs;
            map <int, double> docs_rel;

            for (const string& word : query.p_words) {
                if (!words_id_tf_.count(word) || query.m_words.count(word)) continue;
                double idf = IDFCompute(words_id_tf_.at(word).size());

                for (const auto& [id, tf] : words_id_tf_.at(word)) {
                    if ((docs_rel.count(id) && docs_rel[id] > 0) || !docs_rel.count(id))
                        docs_rel[id] += idf * tf;
                }
            }

            for (const string& word : query.m_words) {
                if (!words_id_tf_.count(word)) continue;
                for (const auto& [id, tf] : words_id_tf_.at(word)) {
                    if ((docs_rel.count(id) && docs_rel[id] > 0) || !docs_rel.count(id))
                        docs_rel.erase(id);
                }
            }

            for (const auto& [id, rel] : docs_rel) {
                if (rel >= 0) matched_docs.push_back({id, rel});
            }

            return matched_docs;
        }

        /**Блок данных сервера.**/
        map <string, map <int, double>> words_id_tf_;
        set <string> stop_words_;
        int doc_count_ = 0;
};

/**Простой инит сервера. Спросим стоп-слова, спросим кол-во доков.
    Спросим все эти доки. А под конец вернём экземпляр готового сервера.**/

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int doc_count = ReadLineWithNumber();
    for (int id = 0; id < doc_count; ++id) {
            search_server.AddDocument(id, ReadLine());
    }

    return search_server;
}


int main() {
    const SearchServer server = CreateSearchServer();

    for (auto [id, rel] : server.FindTopDocuments(ReadLine())) {
        cout << "{ document_id = "s << id << ", relevance = "s << rel << " }"s << endl;
    }

    return 0;
}
