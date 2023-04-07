#include "../header/document.h"

Document::Document() {
    id = 0;
    relevance = 0;
    rating = 0;
}

Document::Document(const int& id_, const double& relevance_, const int rating_)
    :   id(id_),
        relevance(relevance_),
        rating(rating_)
{}

std::ostream& operator<< (std::ostream& os, const Document& document) {
    return os   << "{ document_id = "s  << document.id
                << ", relevance = "s    << document.relevance
                << ", rating = "s       << document.rating
                << " }"s;
}
