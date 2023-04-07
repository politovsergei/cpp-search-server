#include "../header/test_example_functions.h"

void text_example() {
    std::string stop_words = "and with"s;
    std::vector <std::string> doc_lines = {
        "funny pet and nasty rat"s,
        "funny pet with curly hair"s,
        "funny pet with curly hair"s,
        "funny pet and curly hair"s,
        "funny funny pet and nasty nasty rat"s,
        "funny pet and not very nasty rat"s,
        "very nasty rat and not very funny pet"s,
        "pet with rat and rat and rat"s,
        "nasty rat with curly hair"s
    };

    SearchServer search_server(stop_words);

    for (size_t index = 0; index < doc_lines.size(); ++index) {
        index ?
            AddDocument(search_server, index + 1, doc_lines.at(index), DocumentStatus::ACTUAL, {1, 2}) :
            AddDocument(search_server, index + 1, doc_lines.at(index), DocumentStatus::ACTUAL, {7, 2, 7});
    }

    std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;

    RemoveDuplicates(search_server);

    std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
}
