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

        /**Äîáàâëÿåì äîêóìåíòû ïî ïðèíöèïó èíâåðñèè èíäåêñà, TF âû÷èñëÿåòñÿ òóò æå.
            Äèñòðèáóòèâíîå ñâîéñòâî äåëåíèÿ òóò ïðèãîäèòñÿ.**/
    
        void AddDocument(const int document_id, const string& document) {
            const vector <string> query_words = SplitIntoWordsNoStop(document);
            double tf_ratio = 1.0 / static_cast <double> (query_words.size());

            for (const string& word : query_words) {
                    words_id_tf_[word][document_id] += tf_ratio;
            }

            ++doc_count_;
        }

        /**Ôîðìèðóåì âûäà÷ó, ñîãëàñíî ðåëåâàíòíîñòè, ñâåðõó-âíèç.
            Êîëëè÷åñòâî âûäà÷è óñòàíàâëèâàåòñÿ êîíñòàíòîé MAX_RESULT_DOCUMENT_COUNT**/
        vector <Document> FindTopDocuments(const string& raw_query) const {
            vector <Document> docs = FindAllDocuments(ParseQuery(raw_query));

            sort(docs.begin(), docs.end(), [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance; });

            if (docs.size() > MAX_RESULT_DOCUMENT_COUNT)
                docs.resize(MAX_RESULT_DOCUMENT_COUNT);
            return docs;
        }

    private:
        /**Áåç ýòîé ñòðóêòóðû ïîèñê ýôôåêòèâíûì íå ñäåëàòü**/
        struct Query {
            set <string> p_words;
            set <string> m_words;
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

        /**Òåïåðü âñå äîêóìåíòû ïàðñÿòñÿ â ñòðóòóðó.
            Ïëþñ-ñëîâà è ìèíóñ-ñëîâà îòäåëüíî ïî óíèêàëüíûì ñïèñêàì.**/

        Query ParseQuery(const string& text) const {
            Query query;
            for (const string& word : SplitIntoWordsNoStop(text)) {
                word[0] == 45 ? query.m_words.insert(word.substr(1)) : query.p_words.insert(word);
            }

            return query;
        }

        /**Åñëè ïëþñ ñëîâà ïóñòû, òî âåðí¸ì ïóñòîòó.
            Íî âîò åñëè çàïðîñ íå ïóñò, òî ïðîéä¸ìñÿ ïî âñåì ïëþñàì,
            ïîïóòíî ïðîâåðÿÿ, íåò ëè èõ â ìèíóñ-ñëîâàõ, è ïîñ÷èòàåì ðåëåâàíòíîñòü.
            IDF âû÷èñëÿåòñÿ ðàç çà ïðîõîä âíåøíåãî öèêëà, ÷òîáû íå äåëàòü ýòî ìíîãî-ìíîãî ðàç.
            È ïîä ôèíàë âñ¸ ñäîáðèì îòñå÷åíèåì äîêóìåíòîâ ñ ìèíóñàìè.
            ß òóò ïðîñòî óâ¸ë çà íîëü âëåâî ðåëåâàòíîñòü òàêîâûõ.
            Âñ¸ ïåðåïàðñèì ïî óñëîâèþ â âåêòîð ñòðóêòóð è íà âûõîä.
            **/
        vector <Document> FindAllDocuments(const Query& query) const {
            if (query.p_words.size() == 0) return vector <Document>();
            vector <Document> matched_docs;
            map <int, double> docs_rel;
            double idf;

            for (const string& word : query.p_words) {
                if (!words_id_tf_.count(word) || query.m_words.count(word)) continue;
                idf = log(static_cast <double> (doc_count_) /
                          static_cast <double> (words_id_tf_.at(word).size()));

                for (const auto& [id, tf] : words_id_tf_.at(word)) {
                    if ((docs_rel.count(id) && docs_rel[id] > 0) || !docs_rel.count(id))
                        docs_rel[id] += idf * tf;
                }
            }

            for (const string& word : query.m_words) {
                if (!words_id_tf_.count(word)) continue;
                for (const auto& [id, tf] : words_id_tf_.at(word)) {
                    if ((docs_rel.count(id) && docs_rel[id] > 0) || !docs_rel.count(id))
                        docs_rel[id] = -1.0;
                }
            }

            for (const auto& [id, rel] : docs_rel) {
                if (rel >= 0) matched_docs.push_back({id, rel});
            }

            return matched_docs;
        }

        /**Áëîê äàííûõ ñåðâåðà.**/
        map <string, map <int, double>> words_id_tf_;
        set <string> stop_words_;
        int doc_count_ = 0;
};

/**Ïðîñòîé èíèò ñåðâåðà. Ñïðîñèì ñòîï-ñëîâà, ñïðîñèì êîë-âî äîêîâ.
    Ñïðîñèì âñå ýòè äîêè. À ïîä êîíåö âåðí¸ì ýêçåìïëÿð ãîòîâîãî ñåðâåðà.**/
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
