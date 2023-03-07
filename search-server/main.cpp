#include <iostream>
#include <string>
#include <locale.h>

#include "string_processing.h"
#include "document.h"
#include "read_input_functions.h"
#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"

using namespace std::string_literals;

int main()
{
    setlocale(LC_ALL, "Russian");

    TestSearchServer();

    std::set <std::string> query_lines = {"curly dog"s, "big collar"s, "sparrow"s};

    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    {
        search_server.AddDocument(1, "curly cat curly tail"s,       DocumentStatus::ACTUAL, {7, 2, 7});
        search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
        search_server.AddDocument(3, "big cat fancy collar "s,      DocumentStatus::ACTUAL, {1, 2, 8});
        search_server.AddDocument(4, "big dog sparrow Eugene"s,     DocumentStatus::ACTUAL, {1, 3, 2});
        search_server.AddDocument(5, "big dog sparrow Vasiliy"s,    DocumentStatus::ACTUAL, {1, 1, 1});
    }

    for (int index = 0; index < 1439; ++index) {
        request_queue.AddFindRequest("empty request"s);
    }

    for (auto iter_line = query_lines.begin(); iter_line != query_lines.end(); ++iter_line) {
        request_queue.AddFindRequest(*iter_line);
    }

    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    return 0;
}
