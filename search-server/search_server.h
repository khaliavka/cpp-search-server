#pragma once
#include <algorithm>
#include <execution>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "document.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double TOLERANCE = 1e-6;

class SearchServer {
 public:
  template <typename StringContainer>
  explicit SearchServer(const StringContainer& stop_words);
  SearchServer(const std::string& stop_words);

  void AddDocument(int document_id, const std::string& document,
                   DocumentStatus status, const std::vector<int>& ratings);

  void RemoveDocument(int document_id);
  void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
  void RemoveDocument(const std::execution::parallel_policy&, int document_id);

  template <typename DocumentPredicate>
  std::vector<Document> FindTopDocuments(
      const std::string& raw_query, DocumentPredicate document_predicate) const;
  std::vector<Document> FindTopDocuments(const std::string& raw_query,
                                         DocumentStatus status) const;
  std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

  int GetDocumentCount() const;

  std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
      const std::string& raw_query, int document_id) const;

  std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
      const std::execution::sequenced_policy&, const std::string& raw_query,
      int document_id) const;

  std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(
      const std::execution::parallel_policy&, const std::string& raw_query,
      int document_id) const;
      
  const std::map<std::string, double>& GetWordFrequencies(
      int document_id) const;

  std::set<int>::const_iterator begin() const;
  std::set<int>::const_iterator end() const;

 private:
  struct DocumentData {
    int rating;
    DocumentStatus status;
  };

  const std::set<std::string> stop_words_;
  std::map<std::string, std::map<int, double>> word_to_document_freqs_;
  std::map<int, std::map<std::string, double>> document_to_word_freqs_;
  std::map<int, DocumentData> documents_;
  std::set<int> ids_;

  bool IsStopWord(const std::string& word) const;
  bool IsValidStr(const std::string& str) const;
  std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
  int ComputeAverageRating(const std::vector<int>& ratings) const;

  struct QueryWord {
    std::string data;
    bool is_minus;
    bool is_stop;
  };

  QueryWord ParseQueryWord(std::string text) const;

  struct Query {
    std::vector<std::string> plus_words;
    std::vector<std::string> minus_words;
  };

  Query ParseQuery(const std::string& text, bool sort_query = true) const;
  // Existence required
  double ComputeWordInverseDocumentFreq(const std::string& word) const;

  template <typename DocumentPredicate>
  std::vector<Document> FindAllDocuments(
      const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
  using namespace std::literals;

  if (std::any_of(stop_words_.begin(), stop_words_.end(),
                  [this](const std::string& str) {
                    return !(this->IsValidStr(str));
                  })) {
    throw std::invalid_argument("INVALID_SYMBOLS"s);
  }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::string& raw_query, DocumentPredicate document_predicate) const {
  const auto query = ParseQuery(raw_query);
  auto matched_documents = FindAllDocuments(query, document_predicate);

  std::sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
              if (std::abs(lhs.relevance - rhs.relevance) < TOLERANCE) {
                return lhs.rating > rhs.rating;
              }
              return lhs.relevance > rhs.relevance;
            });

  if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
    matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
  }
  return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const Query& query, DocumentPredicate document_predicate) const {
  std::map<int, double> document_to_relevance;

  for (const std::string& word : query.plus_words) {
    if (word_to_document_freqs_.count(word) == 0) {
      continue;
    }
    const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
    for (const auto [document_id, term_freq] :
         word_to_document_freqs_.at(word)) {
      const auto& document_data = documents_.at(document_id);

      if (document_predicate(document_id, document_data.status,
                             document_data.rating)) {
        document_to_relevance[document_id] += term_freq * inverse_document_freq;
      }
    }
  }

  for (const std::string& word : query.minus_words) {
    if (word_to_document_freqs_.count(word) == 0) {
      continue;
    }
    for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
      document_to_relevance.erase(document_id);
    }
  }

  std::vector<Document> matched_documents;
  for (const auto [document_id, relevance] : document_to_relevance) {
    matched_documents.push_back(
        {document_id, relevance, documents_.at(document_id).rating});
  }

  return matched_documents;
}