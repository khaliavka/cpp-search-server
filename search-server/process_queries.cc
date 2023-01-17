#include <algorithm>
#include <execution>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
  std::vector<std::vector<Document>> answers(queries.size());
  std::transform(std::execution::par, queries.cbegin(), queries.cend(),
                 answers.begin(), [&search_server](const std::string& q) {
                   return search_server.FindTopDocuments(q);
                 });
  return answers;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
  std::vector<Document> result;
  for (const auto& documents : ProcessQueries(search_server, queries)) {
    result.insert(result.end(), documents.cbegin(), documents.cend());
  }
  return result;
}