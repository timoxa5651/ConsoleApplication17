#include "Stream.h"

ReadException::ReadException(int position, string message) {
	this->position = position;
	this->msg = message + " at pos " + std::to_string(position + 1);
}


const char* ReadException::what() const noexcept {
	return this->msg.c_str();
}


Stream::Stream(string s) {
	this->str = s;
	this->off = 0;
}

int Stream::get_cur() {
	return this->off;
}
int Stream::get_size() {
	return this->str.size();
}
void Stream::set_cur(int v) {
	this->off = v;
}
void Stream::seek(int c) {
	this->off += c;
}
bool Stream::is_end() {
	return this->off >= this->str.size();
}

char Stream::read_char() {
	return this->str[this->off++];
}
char Stream::peek_char() {
	return this->str[this->off];
}

int64_t Stream::get_num(int st, bool read, bool negat) {
	bool ng = this->str[st] == '-';
	if (!negat && ng)
		throw ReadException(this->off, "Only positive numbers are allowed");
	if (ng) {
		st += 1;
		if (read)
			this->off += 1;
	}

	int64_t rs = 0;
	int64_t rd = 0;
	for (int i = st; i < this->str.size(); ++i) {
		if (this->str[i] >= '0' && this->str[i] <= '9') {
			rd = std::max((int64_t)1, rd * 10);
		}
		else {
			break;
		}
	}
	if (!rd) {
		throw ReadException(this->off, "Number expected");
	}
	for (int i = st; rd; ++i, rd /= 10) {
		rs += (this->str[i] - '0') * rd;
		if (read) {
			this->off += 1;
		}
		st += 1;
	}
	return rs * (ng ? -1 : 1);
}

double Stream::get_num_dbl(int st, bool read, bool negat ) {
	bool ng = this->str[st] == '-';
	if (!negat && ng)
		throw ReadException(this->off, "Only positive numbers are allowed");
	if (ng) {
		st += 1;
		if (read)
			this->off += 1;
	}

	double rs = 0;
	int rd = 0;
	for (int i = st; i < this->str.size(); ++i) {
		if (this->str[i] >= '0' && this->str[i] <= '9') {
			rd = std::max(1, rd * 10);
		}
		else {
			break;
		}
	}
	if (!rd) {
		throw ReadException(this->off, "Number expected");
	}
	for (int i = st; rd; ++i, rd /= 10) {
		rs += (this->str[i] - '0') * rd;
		if (read) {
			this->off += 1;
		}
		st += 1;
	}

	if (this->peek_char() == L'.') {
		if (read)
			this->seek(1);
		double pst = this->get_num(st + 1, read, false);
		while (pst > 1.0) pst /= 10;
		rs += pst;
	}
	return rs * (ng ? -1 : 1);
}
