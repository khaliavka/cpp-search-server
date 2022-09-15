#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {

public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        vector<string> words = SplitIntoWordsNoStop(document);
        auto sz = words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += 1.0 / sz;
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;});
                 
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct Query {
        set<string> plus;
        set<string> minus;
    };

    map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;
    
    int document_count_ = 0;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        vector<string> words = SplitIntoWordsNoStop(text);
        for (const string& word : words) {
            if (word[0] == '-') {
                query_words.minus.insert(word.substr(1));
            }
        }
        for (const string& word : words) {
            if (word[0] != '-' && query_words.minus.count(word) == 0) {
                query_words.plus.insert(word);
            }
        }
        return query_words;
    }

    bool IsOmittedId(int id, const string& word) const {
        if (word_to_document_freqs_.count(word) > 0) {
            return word_to_document_freqs_.at(word).count(id) > 0;
        }
        return false;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        map<int, double> id_rl;
        for (const auto& query_word : query_words.plus) {
            if (word_to_document_freqs_.count(query_word) > 0) {
                auto sz = word_to_document_freqs_.at(query_word).size();
                double idf = log((1.0 * document_count_) / sz);
                for (const auto& [id, tf] : word_to_document_freqs_.at(query_word)) {
                    id_rl[id] += idf * tf;
                } 
            }  
        }
        for (const auto& [id, rl] : id_rl) {
            matched_documents.push_back({id, rl});
            for (const auto& query_word : query_words.minus) {
                if (IsOmittedId(id, query_word)) {
                    matched_documents.pop_back();    
                }
            }          
        }
        return matched_documents;
    }

};
                                           
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
    return 0;
}