#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double TOLERANCE = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
            for (const auto& word : stop_words_) {
                if (!IsValidStr(word)) {
                    throw invalid_argument("CONSTRUCTOR_SYMBOLS"s);
                }
            }
    }

    explicit SearchServer(const string& stop_words)
        : SearchServer(
            SplitIntoWords(stop_words))  { // Invoke delegating constructor from string container
    
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                                    const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("ADD_DOC_NEGATIVE_ID"s);
        }
        if (documents_.count(document_id) == 1) {
            throw invalid_argument("ADD_DOC_SAME_ID"s);
        }
        if (!IsValidStr(document)) {
            throw invalid_argument("ADD_DOC_SYMBOLS"s);
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
                                        DocumentPredicate document_predicate) const {
        if(!IsValidStr(raw_query)) {
            throw invalid_argument("FIND_TOP_DOC_SYMBOLS"s);
        }
        const auto query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < TOLERANCE) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
            });

            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query,
                                        DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        if (documents_.count(index) == 0) {
            throw out_of_range("GET_DOC_ID_OUT_OF_RANGE");  
        }
        return index;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                                    int document_id) const {
        if (!IsValidStr(raw_query)) {
            throw invalid_argument("MATCH_DOC_SYMBOLS"s);
        }
        const auto query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return tuple{matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidStr(const string& str) {
        return none_of(str.begin(), str.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
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

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text.empty()) {
            throw invalid_argument("SINGLE_DASH"s);
        }
        if (text[0] == '-') {
            throw invalid_argument("DOUBLE_DASH"s);
        }
        return QueryWord{text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
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
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
enum class Exception {
    CONSTRUCTOR_SYMBOLS,
    ADD_DOC_SAME_ID,
    ADD_DOC_NEGATIVE_ID,
    ADD_DOC_SYMBOLS,
    FIND_TOP_DOC_SINGLE_DASH,
    FIND_TOP_DOC_DOUBLE_DASH,
    FIND_TOP_DOC_SYMBOLS,
    MATCH_DOC_SINGLE_DASH,
    MATCH_DOC_DOUBLE_DASH,
    MATCH_DOC_SYMBOLS,
    GET_DOC_ID_OUT_OF_RANGE,
    PROPER_DATA
};
int main() {
    Exception key = Exception::CONSTRUCTOR_SYMBOLS;
    switch (key) {
    case Exception::CONSTRUCTOR_SYMBOLS:
        try {
            SearchServer server("и в н\x12а"s);
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::ADD_DOC_SAME_ID:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
            search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::ADD_DOC_NEGATIVE_ID:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::ADD_DOC_SYMBOLS:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2});
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::FIND_TOP_DOC_SINGLE_DASH:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
            search_server.FindTopDocuments("- -пёс"s);
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::FIND_TOP_DOC_DOUBLE_DASH:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
            search_server.FindTopDocuments("--попугай"s);
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::FIND_TOP_DOC_SYMBOLS:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
            search_server.FindTopDocuments("кот скво\x12рец"s);
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::MATCH_DOC_SINGLE_DASH:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
            search_server.MatchDocument("пушистый -"s, 0);
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::MATCH_DOC_DOUBLE_DASH:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
            search_server.MatchDocument("--пушистый"s, 0);
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::MATCH_DOC_SYMBOLS:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
            search_server.MatchDocument("пу\x12шистый"s, 0);
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::GET_DOC_ID_OUT_OF_RANGE:
        try {
            SearchServer search_server("и в на"s);
            for (int i = 0; i < 100; ++i) {
                search_server.AddDocument(i, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {i, i * i});
            }
            search_server.GetDocumentId(404);
        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    case Exception::PROPER_DATA:
        try {
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
            const auto documents = search_server.FindTopDocuments("пушистый"s);
            for (const auto& document : documents) {
                PrintDocument(document);
            }

        } catch (invalid_argument& e) {
            cout << e.what() << endl;
        }
        break;
    }
}