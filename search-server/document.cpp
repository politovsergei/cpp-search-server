#include "document.h"

Document::Document(const int& id_, const double& relevance_, const int rating_)
    : id(id_), relevance(relevance_), rating(rating_) {}

Document::Document() {
    id = 0;
    relevance = 0;
    rating = 0;
}

std::ostream& operator<< (std::ostream& os, const Document& document) {
    os  << "{ "s << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;

    return os;
}
