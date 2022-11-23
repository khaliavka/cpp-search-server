#include "request_queue.h"

#include <string>
#include <vector>

RequestQueue::RequestQueue(const SearchServer& search_server)
    : no_result_requests_(0), search_server_(search_server) {}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentStatus status) {
  std::vector<Document> result =
      search_server_.FindTopDocuments(raw_query, status);
  ProcessQueue(raw_query, result, result.empty(), status);
  return result;
}
std::vector<Document> RequestQueue::AddFindRequest(
    const std::string& raw_query) {
  std::vector<Document> result =
      search_server_.FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
  ProcessQueue(raw_query, result, result.empty(), DocumentStatus::ACTUAL);
  return result;
}
int RequestQueue::GetNoResultRequests() const { return no_result_requests_; }

void RequestQueue::ProcessQueue(const std::string& raw_query,
                                const std::vector<Document>& result,
                                bool is_empty, DocumentStatus status) {
  if (is_empty) ++no_result_requests_;
  requests_.push_back({{raw_query, status}, result, is_empty});
  if (requests_.size() > min_in_day_) {
    if (requests_.front().is_empty) --no_result_requests_;
    requests_.pop_front();
  }
}
