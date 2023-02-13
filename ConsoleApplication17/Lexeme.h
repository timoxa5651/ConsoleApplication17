#pragma once
#include <string>
#include <map>
#include <exception>
#include <stdexcept>
using std::string;

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

struct LexemeSyntax {
	ELexemeType type;

	int line;
	int pos;
	string str_;
	union {
		int64_t i64_;
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
		else if constexpr (std::is_same_v<T, int64_t>) {
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

	LexemeSyntax(ELexemeType t, int line, int pos);

	template<typename T>
	LexemeSyntax(ELexemeType t, const T& value, int line, int pos) : LexemeSyntax(t, line, pos) {
		this->set_val<T>(value);
	};

	~LexemeSyntax();
};


extern string LexemeTypeDataToStr(LexemeSyntax lexeme);
extern std::map< ELexemeType, string> g_LexemeTypeToStr;


class Lexeme {
public:
	ELexemeType type;
	std::string string;
	int64_t line;

	Lexeme();
	Lexeme(ELexemeType lexemeType, std::string string, int64_t line);

	int64_t GetLineLength();
};
