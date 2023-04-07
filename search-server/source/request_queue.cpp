#include "../header/request_queue.h"

RequestQueue::RequestQueue(const SearchServer &search_server)
    : search_server_(search_server), empty_counter_(0) {}

std::vector <Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status) {
    std::vector <Document> matched_documents = search_server_.FindTopDocuments(raw_query, status);

    ChangeStateDeque(matched_documents);

    return matched_documents;
}

std::vector <Document> RequestQueue::AddFindRequest(const std::string &raw_query) {
    std::vector <Document> matched_documents = search_server_.FindTopDocuments(raw_query);

    ChangeStateDeque(matched_documents);

    return matched_documents;
}

void RequestQueue::ChangeStateDeque(const std::vector <Document> &matched_documents) {
    QueryResult result = { matched_documents.empty() };

    empty_counter_ += result.isEmpty;

    requests_.push_front(result);

    if (requests_.size() >= min_in_day_ + 1) {
        empty_counter_ -= requests_.back().isEmpty;
        requests_.pop_back();
    }
}

int RequestQueue::GetNoResultRequests() const {
    return empty_counter_;
}


