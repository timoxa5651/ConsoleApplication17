#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <exception>
#include <fstream>
#include <sstream>

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::exception;



enum class ELexemeType : int {
	Null,
	Invalid,

	Variable,
	Keyword,
	LiteralStr,
	Operator,
	LiteralChar,

	RoundBrack,
	BoxBrack,
	CurlyBrack,
	Punctuation,

	LiteralNum32,
	LiteralNum64,

	LiteralDouble,
};

struct Lexeme {
	ELexemeType type;

	string str_;
	union {
		__int64 i64_;
		int int_;
		float flt_;
	};

	template<typename T>
	T get_val() {
		if constexpr (std::is_same_v<T, std::string>) {
			return this->str_;
		}
		if constexpr (std::is_same_v<T, long long>) {
			return this->i64_;
		}
		if constexpr (std::is_same_v<T, int> || std::is_same_v<T, char>) {
			return this->int_;
		}
		if constexpr (std::is_same_v<T, float>) {
			return this->flt_;
		}
		throw std::logic_error("");
	}
	template<typename T>
	void set_val(const T& val) {
		if constexpr (std::is_same_v<T, std::string>) {
			this->str_ = val;
		}
		else if constexpr (std::is_same_v<T, __int64>) {
			this->i64_ = val;
		}
		else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, char>) {
			this->i64_ = 0;
			this->int_ = val;
		}
		else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
			this->i64_ = 0;
			this->flt_ = val;
		}
		else {
			throw std::logic_error("");
		}
	}

	Lexeme(ELexemeType t) {
		this->type = t;
	};

	template<typename T>
	Lexeme(ELexemeType t, const T& value) {
		this->type = t;
		this->set_val<T>(value);
	};

	~Lexeme() {}
};


static string LexemeTypeDataToStr(Lexeme lexeme) {
	if ((int)lexeme.type >= (int)ELexemeType::LiteralDouble) {
		return std::to_string(lexeme.flt_);
	}
	else if ((int)lexeme.type >= (int)ELexemeType::LiteralNum32) {
		return std::to_string(lexeme.i64_);
	}
	else if ((int)lexeme.type >= (int)ELexemeType::RoundBrack) {
		return std::string() + (char)lexeme.int_;
	}
	else if ((int)lexeme.type >= (int)ELexemeType::Variable) {
		return lexeme.str_;
	}
	return "";
}
static std::map< ELexemeType, string> g_LexemeTypeToStr = {
	{ELexemeType::Null, "Null"},
	{ELexemeType::Invalid, "Invalid"},
	{ELexemeType::Variable, "Variable"},
	{ELexemeType::Keyword, "Keyword"},
	{ELexemeType::LiteralChar, "LiteralChar"},
	{ELexemeType::LiteralDouble, "LiteralDouble"},
	{ELexemeType::LiteralNum32, "LiteralNum32"},
	{ELexemeType::LiteralNum64, "LiteralNum64"},
	{ELexemeType::LiteralStr, "LiteralStr"},
	{ELexemeType::Operator, "Operator"},
	{ELexemeType::Punctuation, "Punctuation"},
	{ELexemeType::RoundBrack, "RoundBrack"},
	{ELexemeType::BoxBrack, "BoxBrack"},
	{ELexemeType::CurlyBrack, "CurlyBrack"}
};



class ReadExpection : exception {
public:
	int position;
	string msg;
	string fmsg;

	ReadExpection(int position, string message = "") {
		this->position = position;
		this->msg = message;
	}


	virtual const char* what() throw()
	{
		this->fmsg = this->msg + " at pos " + std::to_string(position + 1);
		return this->fmsg.c_str();
	}
};

struct Stream { 
public:
	Stream(string s = "") {
		this->str = s;
		this->off = 0;
	}

	int get_cur() {
		return this->off;
	}
	int get_size() {
		return this->str.size();
	}
	void set_cur(int v) {
		this->off = v;
	}
	void seek(int c) {
		this->off += c;
	}
	bool is_end() {
		return this->off >= this->str.size();
	}

	char read_char() {
		return this->str[this->off++];
	}
	char peek_char() {
		return this->str[this->off];
	}

	__int64 get_num(int st, bool read, bool negat = false) {
		bool ng = this->str[st] == '-';
		if (!negat && ng)
			throw ReadExpection(this->off, "Only positive numbers are allowed");
		if (ng) {
			st += 1;
			if (read)
				this->off += 1;
		}

		__int64 rs = 0;
		__int64 rd = 0;
		for (int i = st; i < this->str.size(); ++i) {
			if (this->str[i] >= '0' && this->str[i] <= '9') {
				rd = std::max(1ll, rd * 10);
			}
			else {
				break;
			}
		}
		if (!rd) {
			throw ReadExpection(this->off, "Number expected");
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

	double get_num_dbl(int st, bool read, bool negat = false) {
		bool ng = this->str[st] == '-';
		if (!negat && ng)
			throw ReadExpection(this->off, "Only positive numbers are allowed");
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
			throw ReadExpection(this->off, "Number expected");
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

private:
	string str; // в дс зайди
	int off;
};

int main()
{
	vector<string> resvKeywords/* = { "var", "while", "for", "if", "switch" }*/;
	vector<string> resvOperators = { "+", "*", "/", "-", "%", "=", "&", "==", "!="};

	std::ifstream keyWordsFile("keyWordsFile.txt");
	string str;
	while(keyWordsFile >> str) {
		resvKeywords.push_back(str);
	}
	std::ifstream t("input.txt");
	std::stringstream buffer;
	buffer << t.rdbuf();
	std::string input = buffer.str();

	vector<Lexeme> result;
	result.reserve(100);

	Stream stream = input;
	while (!stream.is_end()) {
		char rd = stream.peek_char();
		if (rd == ' ' || rd == '\n' || rd == '\t') {
			stream.seek(1);
			result.push_back(Lexeme(ELexemeType::Null));
			continue;
		}
		else if (rd == '{' || rd == '}') {
			stream.seek(1);
			result.push_back(Lexeme(ELexemeType::CurlyBrack, rd));
			continue;
		}
		else if (rd == '[' || rd == ']') {
			stream.seek(1);
			result.push_back(Lexeme(ELexemeType::BoxBrack, rd));
			continue;
		}
		else if (rd == '(' || rd == ')') {
			stream.seek(1);
			result.push_back(Lexeme(ELexemeType::RoundBrack, rd));
			continue;
		}
		else if (rd == ',' || rd == ';') {
			stream.seek(1);
			result.push_back(Lexeme(ELexemeType::Punctuation, rd));
			continue;
		}
		else if (rd == '"' || rd == '\'') {
			stream.seek(1);
			string var_name;
			for (int i = 0; !stream.is_end(); ++i) {
				if (stream.peek_char() == rd) break;
				var_name += stream.read_char();
			}
			stream.seek(1);
			result.push_back(Lexeme(rd == '"' ? ELexemeType::LiteralStr : ELexemeType::LiteralChar, var_name));
			continue;
		}
		else if (rd >= '0' && rd <= '9') {
			int cr = stream.get_cur();
			__int64 num = stream.get_num(stream.get_cur(), true, true);
			if (!stream.is_end() && stream.peek_char() == '.') {
				stream.set_cur(cr);
				double num2 = stream.get_num_dbl(stream.get_cur(), true, true);
				result.push_back(Lexeme(ELexemeType::LiteralDouble, num2));
			}
			else {
				if (abs(num) >= INT_MAX) {
					result.push_back(Lexeme(ELexemeType::LiteralNum64, num));
				}
				else {
					result.push_back(Lexeme(ELexemeType::LiteralNum32, num));
				}
			}
			continue;
		}
		else {
			auto checkIfInList = [&](vector<string> list) -> std::tuple<bool, string> {
				int cr = stream.get_cur();
				std::tuple<bool, string> result = { false, "" };
				for (int i = 0; !stream.is_end() && list.size(); ++i) {
					char chr = stream.read_char();
					for (auto it = list.begin(); it != list.end(); ) {
						if (it->at(i) != chr) {
							it = list.erase(it);
						}
						else if (it->size() == i + 1) {
							bool flag = false;
							if (stream.is_end()) flag = true;
							else {
								char c = tolower(stream.peek_char());
								if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')) flag = true;
							}
							if (flag) {
								result = { true, *it };
							}
							it = list.erase(it);
						}
						else {
							++it;
						}
					}
				}
				stream.set_cur(cr);
				return result;
			};

			auto [validKeyword, str] = checkIfInList(resvKeywords);
			if (validKeyword) {
				stream.seek(str.size());
				result.push_back(Lexeme(ELexemeType::Keyword, str));
				continue;
			}
			auto [validOperator, str2] = checkIfInList(resvOperators);
			if (validOperator) {
				stream.seek(str2.size());
				result.push_back(Lexeme(ELexemeType::Operator, str2));
				continue;
			}

			string var_name;
			for (int i = 0; !stream.is_end(); ++i) {
				char c = tolower(stream.peek_char());
				if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')) break;
				var_name += stream.read_char();
			}

			if (!var_name.size()) {
				result.push_back(Lexeme(ELexemeType::Invalid));
				stream.seek(1);
			}
			else {
				result.push_back(Lexeme(ELexemeType::Variable, var_name));
			}
		}
	}

	std::ofstream out("output.txt");
	for (auto rs : result) {
		out << g_LexemeTypeToStr[rs.type] << " " << LexemeTypeDataToStr(rs) << endl;
		//cout << g_LexemeTypeToStr[rs.type] << " " << LexemeTypeDataToStr(rs) << endl;		
	}
	out.close();
	cin.get();
}