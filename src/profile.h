#pragma once
#include <chrono>
#include <iostream>
using namespace std;
using namespace std::chrono;


class LogDuration {
private:
	steady_clock::time_point start;
	const string message;
public:
	explicit LogDuration(const string& mess = "") : start(steady_clock::now()), message (mess + ": "){

	}
	~LogDuration() {
		auto finish = steady_clock::now();
		auto dur = finish - start;
		cerr << message << duration_cast<milliseconds>(dur).count() <<
				" ms" << endl;
	}
};

#define UNIQUE_ID_IMPL(lineno) _a_local_var##lineno
#define UNIQUE_ID(lineno) UNIQUE_ID_IMPL(lineno)

#define LOG_DURATION(msg) \
		LogDuration UNIQUE_ID(__LINE__){msg};
