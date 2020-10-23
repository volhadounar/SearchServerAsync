#include "search_server.h"
#include "parse.h"
#include "test_runner.h"
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <random>
#include <thread>
#include "profile.h"
#include <thread>

#include "profile_advanced.h"
using namespace std;

void TestSearchServer(vector<pair<istringstream, ostringstream*>> streams);
vector<string> GenerateDoc(const size_t doc_cnt, const size_t word_cnt, const size_t word_length);

void TestFunctionality(
  const vector<string>& docs,
  const vector<string>& queries,
  const vector<string>& expected
) {
  istringstream docs_input(Join('\n', docs));
  istringstream queries_input(Join('\n', queries));
  ostringstream queries_output;
  {
	  SearchServer srv;
	  srv.UpdateDocumentBase(docs_input);
	  srv.AddQueriesStream(queries_input, queries_output);
  }
  const string result = queries_output.str();
  const auto lines = SplitBy(Strip(result), '\n');
  ASSERT_EQUAL(lines.size(), expected.size());
  for (size_t i = 0; i < lines.size(); ++i) {
    ASSERT_EQUAL(lines[i], expected[i]);
  }
}

void TestSerpFormat() {

  const vector<string> docs = {
    "london is the capital of great britain",
    "i am travelling down the river"
  };
  const vector<string> queries = {"london", "the"};
  const vector<string> expected = {
    "london: {docid: 0, hitcount: 1}",
    Join(' ', vector{
      "the:",
      "{docid: 0, hitcount: 1}",
      "{docid: 1, hitcount: 1}"
    })
  };

  TestFunctionality(docs, queries, expected);
}

void TestTop5() {
  const vector<string> docs = {
    "milk a",
    "milk b",
    "milk c",
    "milk d",
    "milk e",
    "milk f",
    "milk g",
    "water a",
    "water b",
    "fire and earth"
  };

  const vector<string> queries = {"milk", "water", "rock"};
  const vector<string> expected = {
    Join(' ', vector{
      "milk:",
      "{docid: 0, hitcount: 1}",
      "{docid: 1, hitcount: 1}",
      "{docid: 2, hitcount: 1}",
      "{docid: 3, hitcount: 1}",
      "{docid: 4, hitcount: 1}"
    }),
    Join(' ', vector{
      "water:",
      "{docid: 7, hitcount: 1}",
      "{docid: 8, hitcount: 1}",
    }),
    "rock:",
  };
  TestFunctionality(docs, queries, expected);
}

void TestHitcount() {
  const vector<string> docs = {
    "the river goes through the entire city there is a house near it",
    "the wall",
    "walle",
    "is is is is",
  };
  const vector<string> queries = {"the", "wall", "all", "is", "the is"};
  const vector<string> expected = {
    Join(' ', vector{
      "the:",
      "{docid: 0, hitcount: 2}",
      "{docid: 1, hitcount: 1}",
    }),
    "wall: {docid: 1, hitcount: 1}",
    "all:",
    Join(' ', vector{
      "is:",
      "{docid: 3, hitcount: 4}",
      "{docid: 0, hitcount: 1}",
    }),
    Join(' ', vector{
      "the is:",
      "{docid: 3, hitcount: 4}",
      "{docid: 0, hitcount: 3}",
      "{docid: 1, hitcount: 1}",
    }),
  };
  TestFunctionality(docs, queries, expected);
}

void TestRanking() {
  const vector<string> docs = {
    "london is the capital of great britain",
    "paris is the capital of france",
    "berlin is the capital of germany",
    "rome is the capital of italy",
    "madrid is the capital of spain",
    "lisboa is the capital of portugal",
    "bern is the capital of switzerland",
    "moscow is the capital of russia",
    "kiev is the capital of ukraine",
    "minsk is the capital of belarus",
    "astana is the capital of kazakhstan",
    "beijing is the capital of china",
    "tokyo is the capital of japan",
    "bangkok is the capital of thailand",
    "welcome to moscow the capital of russia the third rome",
    "amsterdam is the capital of netherlands",
    "helsinki is the capital of finland",
    "oslo is the capital of norway",
    "stockgolm is the capital of sweden",
    "riga is the capital of latvia",
    "tallin is the capital of estonia",
    "warsaw is the capital of poland",
  };

  const vector<string> queries = {"moscow is the capital of russia"};
  const vector<string> expected = {
    Join(' ', vector{
      "moscow is the capital of russia:",
      "{docid: 7, hitcount: 6}",
      "{docid: 14, hitcount: 6}",
      "{docid: 0, hitcount: 4}",
      "{docid: 1, hitcount: 4}",
      "{docid: 2, hitcount: 4}",
    })
  };
  TestFunctionality(docs, queries, expected);
}

void TestBasicSearch() {
	  const vector<string> docs = {
	    "we are ready to go",
	    "come on everybody shake you hands",
	    "i love this game",
	    "just like exception safety is not about writing try catch everywhere in your code move semantics are not about typing double ampersand everywhere in your code",
	    "daddy daddy daddy dad dad dad",
	    "tell me the meaning of being lonely",
	    "just keep track of it",
	    "how hard could it be",
	    "it is going to be legen wait for it dary legendary",
	    "we dont need no education"
	  };

	  const vector<string> queries = {
	    "we need some help",
	    "it",
	    "i love this game",
	    "tell me why",
	    "dislike",
	    "about"
	  };

	  const vector<string> expected = {
	    Join(' ', vector{
	      "we need some help:",
	      "{docid: 9, hitcount: 2}",
	      "{docid: 0, hitcount: 1}"
	    }),
	    Join(' ', vector{
	      "it:",
	      "{docid: 8, hitcount: 2}",
	      "{docid: 6, hitcount: 1}",
	      "{docid: 7, hitcount: 1}",
	    }),
	    "i love this game: {docid: 2, hitcount: 4}",
	    "tell me why: {docid: 5, hitcount: 2}",
	    "dislike:",
	    "about: {docid: 3, hitcount: 2}",
	  };
	  TestFunctionality(docs, queries, expected);
}


void TestRuntime() {
	const size_t doc_cnt = 50000;
	const size_t word_cnt = 100;
	const size_t word_length = 10;
	vector<string> ss = GenerateDoc(doc_cnt, word_cnt, word_length);

	 const vector<string> queries = {
			string(word_length, 'a'),
		   // "it",
			string(word_length, 'b'),
			string(word_length, 'c')
	};

	const vector<string> expected = {
			 	 	Join(' ', vector<string> {
				 	string(word_length, 'a')+":",
			 	    "{docid: 0, hitcount: 1}",
			 	    "{docid: 3, hitcount: 1}",
					"{docid: 6, hitcount: 1}",
					"{docid: 9, hitcount: 1}",
					"{docid: 12, hitcount: 1}",
			 	 	}),
					//"it",
			 	    Join(' ', vector<string> {
				 	  string(word_length, 'b')+":",
				 	  "{docid: 1, hitcount: 1}",
				 	  "{docid: 4, hitcount: 1}",
				 	  "{docid: 7, hitcount: 1}",
				 	  "{docid: 10, hitcount: 1}",
				 	  "{docid: 13, hitcount: 1}",
			 	    }),
					Join(' ', vector<string> {
						string(word_length, 'c')+":",
						"{docid: 2, hitcount: 1}",
						"{docid: 5, hitcount: 1}",
						"{docid: 8, hitcount: 1}",
						"{docid: 11, hitcount: 1}",
						"{docid: 14, hitcount: 1}",
					}),
	};
	TestFunctionality(ss, queries, expected);
}

vector<string> GenerateDoc(const size_t doc_cnt, const size_t word_cnt, const size_t word_length) {

		vector<string> ss;
		for (size_t i = 0; i < doc_cnt; i++){
			if (i%3==0){
				string s(word_length, 'a');
				for (size_t j = 0; j < word_cnt; j++){
					if (j % 3 == 0) {
						s += " ";
						s += string(word_length, 'k');
						s += " ";
					} else if (j % 3 == 1){
						s += " ";
					    s += string(word_length, 'l');
					    s += " ";
					} else if (j % 3 == 2){
						s += " ";
						s += string(word_length, 'm');
						s += " ";
					}
				}
				ss.push_back(s);
			} else if (i % 3 == 1) {
				string s(word_length, 'b');
				for (size_t j = 0; j < word_cnt; j++){
						if (j % 3 == 0) {
							s += " ";
							s += string(word_length, 'k');
							s += " ";
						} else if (j % 3 == 1){
							s += " ";
							s += string(word_length, 'l');
							s += " ";
						} else if (j % 3 == 2){
							s += " ";
							s += string(word_length, 'm');
							s += " ";
						}
				}
				ss.push_back(s);
			} else if (i % 3 == 2) {
				string s(word_length, 'c');
				for (size_t i = 0; i < word_cnt; i++){
					if (i % 3 == 0) {
						s += " ";
						s += string(word_length, 'k');
					} else if (i % 3 == 1){
						s += " ";
						s += string(word_length, 'l');
					} else if (i % 3 == 2){
						s += " ";
						s += string(word_length, 'm');
					}
				}
				ss.push_back(s);
			}
		}
		return ss;
}

void TestAsync() {

	vector<pair<istringstream, ostringstream*>> streams;
	for (int i = 0; i < 12; i++) {
		if (i % 2 == 0) {
			auto doc = GenerateDoc(1100, 80, 10);
			istringstream docs_input(Join('\n', doc));
			pair<istringstream, ostringstream*> p(move(docs_input), nullptr);
			streams.push_back(move(p));
		} else {
			auto doc = GenerateDoc(1200, 50, 10);
			istringstream docs_input(Join('\n', doc));
			pair<istringstream, ostringstream*> p(move(docs_input), new ostringstream());
			streams.push_back(move(p));
		}
	}

	TestSearchServer(move(streams));

}

void TestSearchServer(vector<pair<istringstream, ostringstream*>> streams) {
  // IteratorRange Ч шаблон из задачи Paginator
  // random_time() Ч функци€, котора€ возвращает случайный
  // промежуток времени
	{
		LOG_DURATION("Total");
		SearchServer srv(streams.front().first);
		for (auto& [input, output] :
		     IteratorRange(begin(streams) + 1, end(streams))) {
		  this_thread::sleep_for(2s);
		  if (!output) {
		    srv.UpdateDocumentBase(input);
		  } else {
		    srv.AddQueriesStream(input, *output);
		  }
		}

		cout << "end" << endl;
	}
}

int main() {

  TestRunner tr;
  RUN_TEST(tr, TestRuntime);
  RUN_TEST(tr, TestAsync);

  RUN_TEST(tr, TestSerpFormat);
  RUN_TEST(tr, TestTop5);
  RUN_TEST(tr, TestHitcount);
  RUN_TEST(tr, TestRanking);
  RUN_TEST(tr, TestBasicSearch);
}
