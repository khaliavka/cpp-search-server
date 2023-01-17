#include "search_server.h"

#include <iostream>
#include <string>
#include <vector>

#include "process_queries.h"

using namespace std;

int main() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly pet and funny curly and funny rat pet -not -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 3 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 1);
        cout << words.size() << " words for document 2"s << endl;
        // 3 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 1);
        cout << words.size() << " words for document 3"s << endl;
        // 3 words for document 1
    }

    return 0;
}