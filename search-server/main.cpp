// -------- Начало модульных тестов поисковой системы ----------
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1, "FindTopDocuments wrong behavior."s);
        const Document& doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Searching with stop words must be empty."s);
    }
}

void TestAddDocument() {
    {
        SearchServer server;
        server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
        int i = server.GetDocumentCount();
        ASSERT_EQUAL_HINT(i, 1, "Test AddDocument() with GetDocumentCount() failed."s);
        const auto found_docs = server.FindTopDocuments("cat"s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, 42, "Test of id of added document failed."s);

    }

    {
        SearchServer server;
        for (int i = 0; i < 1000; ++i) {
            string content = "cat in the city "s + to_string(i);
            server.AddDocument(i, content, DocumentStatus::ACTUAL, {1, 2, 3});
            ASSERT_EQUAL_HINT(server.GetDocumentCount(), i + 1, "Test of adding of large quantity of documents failed."s);
        }
    }

}

void TestMinusWords() {
    
    {
        SearchServer server;
        server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
        const auto found_docs0 = server.FindTopDocuments("in the city -dog"s);
        ASSERT_HINT(found_docs0.size() == 1, "One of two documents must be omitted."s);
        const Document& doc0 = found_docs0[0];
        ASSERT_HINT(doc0.id == 42, "Id check of found document failed."s);

        const auto found_docs1 = server.FindTopDocuments("city -cat"s);
        ASSERT_HINT(found_docs1.size() == 1, "One of two documents must be omitted."s);
        const Document& doc1 = found_docs1[0];
        ASSERT_HINT(doc1.id == 43, "Id check of found document failed."s);
    }
}

void TestDocumentMatching() {
    {
        SearchServer server;
        server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(43, "dog in the town"s, DocumentStatus::BANNED, {1, 2, 3});
        
    {
        vector<string> matched_words_ref = {"cat"s, "city"s, "in"s, "the"s};
        const auto [matched_words, status] = server.MatchDocument("cat in the city"s, 42);
        ASSERT_EQUAL_HINT(matched_words, matched_words_ref, "MatchDocument() doesn't behave as expected."s);
        ASSERT(status == DocumentStatus::ACTUAL);
    }
    
    {    
        vector<string> matched_words_ref = {"in"s, "the"s};
        const auto [matched_words, status] = server.MatchDocument("cat in the city"s, 43);
        ASSERT_EQUAL_HINT(matched_words, matched_words_ref, "MatchDocument() doesn't behave as expected."s);
        ASSERT(status == DocumentStatus::BANNED);
    }
    
    {    
        vector<string> matched_words_ref = {"dog"s, "in"s, "the"s};
        const auto [matched_words, status] = server.MatchDocument("dog in the city"s, 43);
        ASSERT_EQUAL_HINT(matched_words, matched_words_ref, "MatchDocument() doesn't behave as expected."s);
        ASSERT(status == DocumentStatus::BANNED);
    }
    //Test MatchDocument with stop words in query.
    {    
        vector<string> matched_words_ref = {};
        const auto [matched_words, status] = server.MatchDocument("dog in the city -dog"s, 43);
        ASSERT_EQUAL_HINT(matched_words, matched_words_ref, "MatchDocument() doesn't behave as expected."s);
        ASSERT(status == DocumentStatus::BANNED);
    }    
    }
}

void TestSortByRelevance() {
    {
        SearchServer server;
        server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(43, "dog beneath the bridge"s, DocumentStatus::ACTUAL, {1, 2, 6});
        server.AddDocument(44, "parrot in the village"s, DocumentStatus::ACTUAL, {1, 2, 9});
        const auto search_1 = server.FindTopDocuments("in the city"s);
        bool order_1 = abs(search_1[0].relevance - search_1[1].relevance) < TOLERANCE ||
            search_1[0].relevance > search_1[1].relevance;
        bool order_2 = abs(search_1[1].relevance - search_1[2].relevance) < TOLERANCE ||
            search_1[1].relevance > search_1[2].relevance;
        ASSERT_HINT(order_1, "Wrong sort by relevance. Check FindTopDocuments(). Also check TOLERANCE constant."s);
        ASSERT_HINT(order_2, "Wrong sort by relevance. Check FindTopDocuments(). Also check TOLERANCE constant."s);
    }
    {
        SearchServer server;
        server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(43, "dog beneath the bridge"s, DocumentStatus::ACTUAL, {1, 2, 6});
        server.AddDocument(44, "parrot in the village"s, DocumentStatus::ACTUAL, {1, 2, 9});
        const auto search_1 = server.FindTopDocuments(" dog  parrot  cat "s);
        bool order_1 = abs(search_1[0].relevance - search_1[1].relevance) < TOLERANCE ||
            search_1[0].relevance > search_1[1].relevance;
        bool order_2 = abs(search_1[1].relevance - search_1[2].relevance) < TOLERANCE ||
            search_1[1].relevance > search_1[2].relevance;
        ASSERT_HINT(order_1, "Wrong sort by relevance. Check FindTopDocuments(). Also check TOLERANCE constant."s);
        ASSERT_HINT(order_2, "Wrong sort by relevance. Check FindTopDocuments(). Also check TOLERANCE constant."s);
    }


}

void TestAverageRatingComputation() {
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 2});
    server.AddDocument(43, "dog beneath the bridge"s, DocumentStatus::ACTUAL, {1, 2, 8});
    server.AddDocument(44, "parrot in the village"s, DocumentStatus::ACTUAL, {1, 2, 9});
    auto search_1 = server.FindTopDocuments(" parrot "s);
    ASSERT_EQUAL(search_1[0].rating, 4);
    search_1 = server.FindTopDocuments("dog"s);
    ASSERT_EQUAL(search_1[0].rating, 3);
    search_1 = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(search_1[0].rating, 1);
}

void TestCustomFilteringWithPredicate() {
    SearchServer server;
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 2});
    server.AddDocument(43, "dog beneath the bridge"s, DocumentStatus::IRRELEVANT, {1, 2, 8});
    server.AddDocument(44, "parrot in the village"s, DocumentStatus::BANNED, {1, 2, 9});
    auto search_1 = server.FindTopDocuments("the"s);
    ASSERT_EQUAL_HINT(search_1[0].id, 42, "Test of TopDocuments(string) version failed"s);
    search_1 = server.FindTopDocuments("the"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL_HINT(search_1[0].id, 43, "Test of TopDocuments(string, DocumentStatus) version failed"s);
    search_1 = server.FindTopDocuments("the"s, [](int id, DocumentStatus st, int rating) { return id == 44;});
    ASSERT_EQUAL_HINT(search_1[0].id, 44, "Test of TopDocuments(string, Predicate) version failed"s);
    search_1 = server.FindTopDocuments("the"s, [](int id, DocumentStatus st, int rating) { return id == 404;});
    ASSERT_HINT(search_1.empty(), "Test of TopDocuments(string, Predicate) version failed"s);
}

void TestRelevanceComputation() {
    SearchServer server;
    server.AddDocument(42, "cat in the city of Stambul"s, DocumentStatus::ACTUAL, {1, 2, 2});
    server.AddDocument(43, "dog beneath the bridge"s, DocumentStatus::ACTUAL, {1, 2, 8});
    server.AddDocument(44, "parrot in the village"s, DocumentStatus::ACTUAL, {1, 2, 9});
    auto search = server.FindTopDocuments("cat city dog parrot in village");
    {
        bool is_in_tolerance = abs(search[0].relevance - 0.650672) < TOLERANCE;
        ASSERT_HINT(is_in_tolerance, "Check relevance computation or tolerance constant.");
    }
    {
        bool is_in_tolerance = abs(search[1].relevance - 0.433781) < TOLERANCE;
        ASSERT_HINT(is_in_tolerance, "Check relevance computation or tolerance constant.");
    }
    {
        bool is_in_tolerance = abs(search[2].relevance - 0.274653) < TOLERANCE;
        ASSERT_HINT(is_in_tolerance, "Check relevance computation or tolerance constant.");
    }
    
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestDocumentMatching);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestAverageRatingComputation);
    RUN_TEST(TestCustomFilteringWithPredicate);
    RUN_TEST(TestRelevanceComputation);
}

// --------- Окончание модульных тестов поисковой системы -----------
