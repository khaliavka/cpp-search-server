#include <map>
#include <string>
#include <vector>

#include "search_server.h"

void RemoveDuplicates(SearchServer& search_server) {
  std::map<std::string, std::vector<int>> duplicates;
  for (const int document_id : search_server) {
    const auto& word_freqs_ = search_server.GetWordFrequencies(document_id);
    std::string key;
    for (const auto& [word, _] : word_freqs_) {
      key += word;
    }
    duplicates[key].push_back(document_id);
  }
  for (const auto& [_, ids] : duplicates) {
    for (auto it = ids.cbegin() + 1; it != ids.cend(); ++it) {
      search_server.RemoveDocument(*it);
    }
  }
}