#pragma once

#include <iostream>
#include <string>

using namespace std::string_literals;

enum class DocumentStatus { ACTUAL, IRRELEVANT, BANNED, REMOVED };

struct Document {
    Document(const int& id_, const double& relevance_, const int rating_);
    Document();

    int id;
    double relevance;
    int rating;
};

std::ostream& operator<< (std::ostream& os, const Document& document);
