#pragma once
#include <string>
#include <exception>
#include <fstream>
#include <vector>

using std::vector;
using std::string;

class ReadException : std::exception {
public:
	int position;
	string msg;

	ReadException(int position, string message = "");
	virtual const char* what() const noexcept;
};

struct Stream {
public:
	Stream(string s = "");

	int get_cur();
	int get_size();
	void set_cur(int v);
	void seek(int c);
	bool is_end();
	char read_char();
	char peek_char();

	int64_t get_num(int st, bool read, bool negat = false);

	double get_num_dbl(int st, bool read, bool negat = false);

private:
	string str; // � �� �����
	int off;
};
