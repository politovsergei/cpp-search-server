![yp_cpp](https://i.imgur.com/sUYi5se.jpeg)

# Search Engine
Данный поисковый движок представляет собой классического представителя тех поисковых машин, которые вы можете встретить сегодня в интернете. 
На момент данного коммита он умеет обрабатывать поисковые запросы с учётом минус-слов, исключая документы из поисковый выдачи.
Сама поисковая выдача ранжируется как алгоритмом TF-IDF, так и заданными параметрами запроса к API.

## Функционал:
* обработка и хранение текстовых документов принципом обратной индексации
* определение стоп слов при инициализации с последующим исключением из посковой выдачи
* работа с очередями запросов
* постраничный вывод результатов поиска
* обработка минус-слов с последующим исключением документов из поисковой выдачи
* обработка и хранение рейтинга документов
* определение статуса документов

## Требования в сборке
* Windows/Linux
* MinGW/GCC 8.0+

> Флаги сборки: -Werror -Wall -std=c++17

## API
> ВНИМАНИЕ: Интерфейс работает только с текстовыми данными в пространстве UTF-8

Конструкторы сервера:

        template <typename StringCollection>
        explicit SearchServer(const StringCollection& stop_words);
        explicit SearchServer(const std::string& text);

Добавление и удаление документов:

        void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector <int>& ratings);
        void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);
        void RemoveDocument(int document_id);

Поиск по базе документов:
        
        template <typename KeyMapper>
        std::vector <Document> FindTopDocuments(const std::string& raw_query, KeyMapper k_mapper) const;
        std::vector <Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
        std::vector <Document> FindTopDocuments(const std::string& raw_query) const;

Предикаты поиска:

        bool prediction(int document_id, DocumentStatus doc_status, int rating);
        enum class DocumentStatus { ACTUAL, IRRELEVANT, BANNED, REMOVED };

Получение количества документов в базе сервера:
        
        int GetDocumentCount()

Итераторы множества ID:

        std::set <int> ::const_iterator begin();
        std::set <int> ::const_iterator end();
  
## План по развитию:
* Поддержка многопоточности
* Поддержка графического интерфейса
* Поддержка протокола HTTPS
