#pragma once

#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

class RequestQueue {
public:
    RequestQueue(const SearchServer& search_server);
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
        ProcessQueue(raw_query, result, result.empty(), DocumentStatus::ACTUAL);
        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryData {
        std::string raw_query;
        DocumentStatus status;
    };
    struct QueryResult {
        QueryData query;
        std::vector<Document> result;
        bool is_empty;
    };
    const static size_t min_in_day_ = BUFFER_SIZE;
    int no_result_requests_;
    std::deque<QueryResult> requests_;
    const SearchServer& search_server_;
    void ProcessQueue(const std::string& raw_query, const std::vector<Document>& result, bool is_empty, DocumentStatus status);
};