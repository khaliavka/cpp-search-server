#include <iostream>

#include "document.h"
#include "log_duration.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"

using namespace std;

const int PAGE_SIZE = 3;

int main() {
  SearchServer search_server("and in at"s);
  RequestQueue request_queue(search_server);
  search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL,
                            {7, 2, 7});
  search_server.AddDocument(2, "curly dog and fancy collar"s,
                            DocumentStatus::ACTUAL, {1, 2, 3});
  search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL,
                            {1, 2, 8});
  search_server.AddDocument(4, "big dog sparrow Eugene"s,
                            DocumentStatus::ACTUAL, {1, 3, 2});
  search_server.AddDocument(5, "big dog sparrow Vasiliy"s,
                            DocumentStatus::ACTUAL, {1, 1, 1});
  search_server.AddDocument(6, "big dog sparrow Vasiliy"s,
                            DocumentStatus::ACTUAL, {1, 1, 1});
  search_server.AddDocument(7, "big dog sparrow Vasiliy"s,
                            DocumentStatus::ACTUAL, {1, 1, 1});
  {
    LOG_DURATION_STREAM("Operation time"s, cout);
    for (int i = 8; i < 100; ++i) {
      search_server.AddDocument(i, "big dog sparrow Vasiliy"s,
                                DocumentStatus::ACTUAL, {1, 1, 1});
    }
    const auto s_r = search_server.FindTopDocuments("curly dog"s);
    const auto pages = Paginate(s_r, PAGE_SIZE);
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
      std::cout << *page << std::endl;
      std::cout << "Page break"s << std::endl;
    }
  }
  RemoveDuplicates(search_server);
  std::cout << search_server.GetDocumentCount() << endl;
  const auto search_results = search_server.FindTopDocuments("curly dog"s);
  const auto pages = Paginate(search_results, PAGE_SIZE);
  // Выводим найденные документы по страницам
  for (auto page = pages.begin(); page != pages.end(); ++page) {
    std::cout << *page << std::endl;
    std::cout << "Page break"s << std::endl;
  }
  // 1439 запросов с нулевым результатом
  for (int i = 0; i < 1439; ++i) {
    request_queue.AddFindRequest("empty request"s);
  }
  // все еще 1439 запросов с нулевым результатом
  request_queue.AddFindRequest("curly dog"s);
  // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
  request_queue.AddFindRequest("big collar"s);
  // первый запрос удален, 1437 запросов с нулевым результатом
  request_queue.AddFindRequest("sparrow"s);
  request_queue.AddFindRequest("fancy sparrow"s);
  std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests()
            << std::endl;
  return 0;
}