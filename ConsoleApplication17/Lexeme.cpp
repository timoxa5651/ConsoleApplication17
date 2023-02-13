#include "Lexeme.h"

LexemeSyntax::LexemeSyntax(ELexemeType t, int line, int pos) {
	this->type = t;
	this->line = line;
	this->pos = pos;
};


LexemeSyntax::~LexemeSyntax() {}


Lexeme::Lexeme() {
	type = ELexemeType::Null;
	string = "";
	line = 0;
}

Lexeme::Lexeme(ELexemeType lexemeType, std::string string, int64_t line) {
	this->type = lexemeType;
	this->string = std::move(string);
	this->line = line;
}

int64_t Lexeme::GetLineLength() {
	return string.size() + g_LexemeTypeToStr[type].size() + 2 + std::to_string(line).size();
}



string LexemeTypeDataToStr(LexemeSyntax lexeme) {
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
std::map< ELexemeType, string> g_LexemeTypeToStr = {
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
