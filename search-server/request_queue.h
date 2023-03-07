#pragma once

#include <vector>
#include <string>
#include <deque>

#include "search_server.h"
#include "document.h"

class RequestQueue {
    public:
        explicit RequestQueue(const SearchServer &search_server);

        template <typename DocumentPredicate>
        std::vector <Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate);

        std::vector <Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);
        std::vector <Document> AddFindRequest(const std::string &raw_query);

        void ChangeStateDeque(const std::vector <Document> &matched_documents);

        int GetNoResultRequests() const;

    private:
        struct QueryResult {
            bool isEmpty;
        };

        //DATA
        std::deque <QueryResult> requests_;
        const static int min_in_day_ = 1440;
        const SearchServer& search_server_;
        int empty_counter_;
};
