#include <iostream>

#include "../header/remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::vector <int> id_for_remove;
    std::map <std::set <std::string>, int> words_and_id;

    for (const int id : search_server) {
        std::set <std::string> words;

        for (const auto& [word, freq] : search_server.GetWordFrequencies(id)) {
            words.insert(word);
        }

        // Не совсем понял с этим моментом, т.к. контейнер и так set, а вот id выше нужно искать всеравно
        if (words_and_id.find(words) != words_and_id.end()) {
            if (words_and_id.at(words) > id) {
                id_for_remove.push_back(words_and_id.at(words));

                std::cout << "Found duplicate document id "s << words_and_id.at(words) << std::endl;
                words_and_id[words] = id;
            } else {

                std::cout << "Found duplicate document id "s << id << std::endl;
                id_for_remove.push_back(id);
            }
        } else {
            words_and_id[words] = id;
        }
    }

    std::for_each(id_for_remove.begin(), id_for_remove.end(), [&search_server] (const int id) { search_server.RemoveDocument(id); });
}
