#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "string_processing.h"

SearchServer::SearchServer(const std::string& stop_words)
    : SearchServer(SplitIntoWords(stop_words)) {
}

void SearchServer::AddDocument(int document_id, const std::string& document,
                               DocumentStatus status,
                               const std::vector<int>& ratings) {
  using namespace std::literals;
  if (document_id < 0) {
    throw std::invalid_argument("ADD_DOC_NEGATIVE_ID"s);
  }
  if (documents_.count(document_id) == 1) {
    throw std::invalid_argument("ADD_DOC_SAME_ID"s);
  }
  if (!IsValidStr(document)) {
    throw std::invalid_argument("INVALID_SYMBOLS"s);
  }

  const std::vector<std::string> words = SplitIntoWordsNoStop(document);
  const double inv_word_count = 1.0 / words.size();

  for (const std::string& word : words) {
    word_to_document_freqs_[word][document_id] += inv_word_count;
    document_to_word_freqs_[document_id][word] += inv_word_count;
  }

  documents_.emplace(document_id,
                     DocumentData{ComputeAverageRating(ratings), status});
  ids_.emplace(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
  if (ids_.count(document_id) == 0) {
    return;
  }

  const auto& words = document_to_word_freqs_.at(document_id);
  for (const auto& word : words) {
    word_to_document_freqs_.at(word.first).erase(document_id);
  }

  document_to_word_freqs_.erase(document_id);
  documents_.erase(document_id);
  ids_.erase(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(
    const std::string& raw_query, DocumentStatus status) const {
  return FindTopDocuments(
      raw_query, [status](int /*document_id*/,
                          DocumentStatus document_status, int /*rating*/) {
        return status == document_status;
      });
}

std::vector<Document> SearchServer::FindTopDocuments(
    const std::string& raw_query) const {
  return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
  return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus>
SearchServer::MatchDocument(const std::string& raw_query,
                            int document_id) const {
  const auto query = ParseQuery(raw_query);
  std::vector<std::string> matched_words;
  
  for (const std::string& word : query.plus_words) {
    if (word_to_document_freqs_.count(word) == 0) {
      continue;
    }
    if (word_to_document_freqs_.at(word).count(document_id)) {
      matched_words.push_back(word);
    }
  }

  for (const std::string& word : query.minus_words) {
    if (word_to_document_freqs_.count(word) == 0) {
      continue;
    }
    if (word_to_document_freqs_.at(word).count(document_id)) {
      matched_words.clear();
      break;
    }
  }

  return std::tuple{matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string& word) const {
  return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidStr(const std::string& str) const {
  return std::none_of(str.begin(), str.end(),
                      [](char c) { return c >= '\0' && c < ' '; });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(
    const std::string& text) const {
  std::vector<std::string> words;
  for (const std::string& word : SplitIntoWords(text)) {
    if (!IsStopWord(word)) {
      words.push_back(word);
    }
  }
  return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) const {
  if (ratings.empty()) {
    return 0;
  }

  const int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
  return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
  using namespace std::literals;
  bool is_minus = false;
  // Word shouldn't be empty
  if (text[0] == '-') {
    is_minus = true;
    text = text.substr(1);
  }
  if (text.empty()) {
    throw std::invalid_argument("SINGLE_DASH"s);
  }
  if (text[0] == '-') {
    throw std::invalid_argument("DOUBLE_DASH"s);
  }
  return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
  using namespace std::literals;
  if (!IsValidStr(text)) {
    throw std::invalid_argument("INVALID_SYMBOLS"s);
  }
  Query query;
  for (const std::string& word : SplitIntoWords(text)) {
    const auto query_word = ParseQueryWord(word);
    if (!query_word.is_stop) {
      if (query_word.is_minus) {
        query.minus_words.insert(query_word.data);
      } else {
        query.plus_words.insert(query_word.data);
      }
    }
  }
  return query;
}
// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(
    const std::string& word) const {
  return std::log(GetDocumentCount() * 1.0 /
                  word_to_document_freqs_.at(word).size());
}

std::set<int>::const_iterator SearchServer::begin() const {
  return ids_.cbegin();
}

std::set<int>::const_iterator SearchServer::end() const {
  return ids_.cend();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(
    int document_id) const {
  if (document_to_word_freqs_.count(document_id) != 0) {
    return document_to_word_freqs_.at(document_id);
  }
  
  static const std::map<std::string, double> dummy;
  return dummy;
}
