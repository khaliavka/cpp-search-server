#pragma once
#include <algorithm>
#include <execution>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "concurrent_map.h"
#include "document.h"
#include "string_processing.h"

constexpr size_t MAX_RESULT_DOCUMENT_COUNT = 5ull;
constexpr size_t BUCKET_COUNT = 100ull;
constexpr double REL_TOLERANCE = 1e-6;

class SearchServer {
 public:
  template <typename StringContainer>
  explicit SearchServer(const StringContainer& stop_words);
  SearchServer(const std::string& stop_words);
  SearchServer(std::string_view stop_words);

  void AddDocument(int document_id, std::string_view document,
                   DocumentStatus status, const std::vector<int>& ratings);

  void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
  void RemoveDocument(const std::execution::parallel_policy&, int document_id);
  void RemoveDocument(int document_id);
  // 1
  template <
      typename DocumentPredicate,
      typename std::enable_if_t<
          !std::is_same_v<DocumentPredicate, DocumentStatus>, bool> = false>
  std::vector<Document> FindTopDocuments(
      const std::execution::sequenced_policy&, std::string_view raw_query,
      DocumentPredicate document_predicate) const;
  // 2
  template <
      typename DocumentPredicate,
      typename std::enable_if_t<
          !std::is_same_v<DocumentPredicate, DocumentStatus>, bool> = false>
  std::vector<Document> FindTopDocuments(
      const std::execution::parallel_policy&, std::string_view raw_query,
      DocumentPredicate document_predicate) const;
  // 3
  template <typename ExecutionPolicy>
  std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy,
                                         std::string_view raw_query,
                                         DocumentStatus status) const;
  // 4
  template <typename ExecutionPolicy>
  std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy,
                                         std::string_view raw_query) const;
  // 5
  template <typename DocumentPredicate>
  std::vector<Document> FindTopDocuments(
      std::string_view raw_query, DocumentPredicate document_predicate) const;
  // 6
  std::vector<Document> FindTopDocuments(std::string_view raw_query,
                                         DocumentStatus status) const;
  // 7
  std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

  size_t GetDocumentCount() const;

  std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
      const std::execution::sequenced_policy&, std::string_view raw_query,
      int document_id) const;

  std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
      const std::execution::parallel_policy&, std::string_view raw_query,
      int document_id) const;

  std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
      std::string_view raw_query, int document_id) const;

  const std::map<std::string_view, double>& GetWordFrequencies(
      int document_id) const;

  std::set<int>::const_iterator begin() const;
  std::set<int>::const_iterator end() const;

 private:
  struct DocumentData {
    int rating;
    DocumentStatus status;
  };

  const std::set<std::string, std::less<>> stop_words_;
  std::map<int, std::string> raw_documents_;
  std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
  std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
  std::map<int, DocumentData> documents_;
  std::set<int> ids_;

  bool IsStopWord(std::string_view word) const;
  bool IsValidStr(std::string_view str) const;

  std::vector<std::string_view> SplitIntoWordsNoStop(
      std::string_view text) const;

  int ComputeAverageRating(const std::vector<int>& ratings) const;

  struct QueryWord {
    std::string_view data;
    bool is_minus;
    bool is_stop;
  };

  QueryWord ParseQueryWord(std::string_view text) const;

  struct Query {
    std::vector<std::string_view> plus_words;
    std::vector<std::string_view> minus_words;
  };

  Query ParseQuery(std::string_view raw_query, bool sort = true) const;

  double ComputeWordInverseDocumentFreq(std::string_view word) const;

  template <typename DocumentPredicate>
  std::vector<Document> FindAllDocuments(
      const std::execution::sequenced_policy&, const Query& query,
      DocumentPredicate document_predicate) const;

  template <typename DocumentPredicate>
  std::vector<Document> FindAllDocuments(
      const std::execution::parallel_policy&, const Query& query,
      DocumentPredicate document_predicate) const;

  void RemoveDocumentInternal(int document_id);
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(std::move(MakeUniqueNonEmptyStrings(stop_words))) {
  using namespace std::literals;

  if (std::any_of(stop_words_.begin(), stop_words_.end(),
                  [this](const std::string& str) {
                    return !(this->IsValidStr(str));
                  })) {
    throw std::invalid_argument("INVALID_SYMBOLS"s);
  }
}
// 1
template <typename DocumentPredicate,
          typename std::enable_if_t<
              !std::is_same_v<DocumentPredicate, DocumentStatus>, bool>>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::execution::sequenced_policy&, std::string_view raw_query,
    DocumentPredicate document_predicate) const {
  const auto query = ParseQuery(raw_query);
  auto matched_documents =
      FindAllDocuments(std::execution::seq, query, document_predicate);

  std::sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
              if (std::abs(lhs.relevance - rhs.relevance) < REL_TOLERANCE) {
                return lhs.rating > rhs.rating;
              }
              return lhs.relevance > rhs.relevance;
            });

  if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
    matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
  }
  return matched_documents;
}
// 2
template <typename DocumentPredicate,
          typename std::enable_if_t<
              !std::is_same_v<DocumentPredicate, DocumentStatus>, bool>>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::execution::parallel_policy&, std::string_view raw_query,
    DocumentPredicate document_predicate) const {
  const auto query = ParseQuery(raw_query);
  auto matched_documents =
      FindAllDocuments(std::execution::par, query, document_predicate);

  std::sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
              if (std::abs(lhs.relevance - rhs.relevance) < REL_TOLERANCE) {
                return lhs.rating > rhs.rating;
              }
              return lhs.relevance > rhs.relevance;
            });

  if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
    matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
  }
  return matched_documents;
}
// 3
template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    const ExecutionPolicy& policy, std::string_view raw_query,
    DocumentStatus status) const {
  return FindTopDocuments(
      policy, raw_query,
      [status](int /*document_id*/, DocumentStatus document_status,
               int /*rating*/) {
        return status == document_status;
      });  // 1 or 2 according to policy
}
// 4
template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    const ExecutionPolicy& policy, std::string_view raw_query) const {
  return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);  // 3
}
// 5
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    std::string_view raw_query, DocumentPredicate document_predicate) const {
  return FindTopDocuments(std::execution::seq, raw_query,
                          document_predicate);  // 1
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const std::execution::sequenced_policy&, const Query& query,
    DocumentPredicate document_predicate) const {
  std::map<int, double> document_to_relevance;

  for (std::string_view word : query.plus_words) {
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
  /*
  for (std::string_view word : query.minus_words) {
    if (word_to_document_freqs_.count(word) == 0) {
      continue;
    }
    for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
      document_to_relevance.erase(document_id);
    }
  }
  */
  std::vector<Document> matched_documents;
  for (const auto [document_id, relevance] : document_to_relevance) {
    matched_documents.push_back(
        {document_id, relevance, documents_.at(document_id).rating});
    for (std::string_view word : query.minus_words) {
      if (word_to_document_freqs_.count(word) != 0 &&
          word_to_document_freqs_.at(word).count(document_id) != 0) {
        matched_documents.pop_back();
      }
    }
  }
  return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const std::execution::parallel_policy&, const Query& query,
    DocumentPredicate document_predicate) const {
      
  ConcurrentMap<int, double> document_to_relevance(BUCKET_COUNT);

  for_each(
      std::execution::par, query.plus_words.cbegin(), query.plus_words.cend(),
      [this, &document_predicate, &document_to_relevance](const auto& word) {
        if (word_to_document_freqs_.count(word) == 0) {
          return;
        }
        const double inverse_document_freq =
            ComputeWordInverseDocumentFreq(word);

        for (const auto [document_id, term_freq] :
             word_to_document_freqs_.at(word)) {
          const auto& document_data = documents_.at(document_id);

          if (document_predicate(document_id, document_data.status,
                                 document_data.rating)) {
            document_to_relevance[document_id].ref_to_value +=
                term_freq * inverse_document_freq;
          }
        }
        return;
      });

  for_each(
      query.minus_words.cbegin(), query.minus_words.cend(),
      [this, &document_to_relevance](const auto& word) {
        if (word_to_document_freqs_.count(word) == 0) {
          return;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
          document_to_relevance.Erase(document_id);
        }
        return;
      });

  std::vector<Document> matched_documents;

  for (const auto [document_id, relevance] :
       document_to_relevance.BuildOrdinaryMap()) {
    matched_documents.push_back(
        {document_id, relevance, documents_.at(document_id).rating});
  }

  return matched_documents;
}