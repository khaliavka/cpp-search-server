#include "document.h"

#include <iostream>
#include <string>

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating) {}

std::ostream& operator<<(std::ostream& os, const Document& d) {
  using namespace std;
  os << "{ "s
     << "document_id = "s << d.id << ", "s
     << "relevance = "s << d.relevance << ", "s
     << "rating = "s << d.rating << " }"s;
  return os;
}