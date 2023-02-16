#pragma once
#include <string>
#include <vector>
#include <exception>
#include <set>
#include "Lexeme.h"
#include "Stream.h"

using std::vector;
using std::string;

class ParserException : std::exception {
public:
	ELexemeType lexemeType;
	std::string lexeme;
	int64_t lexemeNum;
	std::string context;
	int64_t line;

	ParserException();

	ParserException(ELexemeType lexemeType, const std::string& lexeme, int64_t lexemeNum, const std::string& context, int64_t line);

	ParserException(const Lexeme& lexeme, int64_t lexemeNum, const std::string& context);

	virtual std::string what();
};

struct DeclaredFunction {
	string name;
	int numArgs;

	bool operator==(const DeclaredFunction& ex) {
		return ex.name == this->name;
	}
};

class CompilationResult;
class FunctionScope;
class Parser {
public:
	explicit Parser(const vector<LexemeSyntax>& lexemes);
	~Parser();

	bool Check(CompilationResult* result);
private:
	vector<Lexeme> input_;
	int currentLexemeIdx;
	Lexeme curLexeme_;
	ParserException deepestException;
	string currentClass;
	std::map<string, std::set<DeclaredFunction>> declaredFunctions;
	std::set<string> declaredClasses;
	FunctionScope* currentScope;
	bool isInAssign = false;
	bool isInFuncCall = false;

	void MovePtr(int idx);

    void ClassBlock();
    void ClassStatement();
	void ReadLexeme();
	void Program();
	void Function();
	void FunctionArgumentsDeclaration();
	void Block();
	void Statement();
	void ValueExp();
	void Operand();
	void Value();
	void Priority1();
	void Priority2();
	void Priority3();
	void Priority4();
	void Priority5();
	void Priority6();
	void Priority7();
	void Priority8();
	void FunctionCall();
	void Arguments();
	void Container();
	void String();
	void Name();
	void Num();
	void List();
	void ListElement();
	void Exp();
	void Return();
	void Assign();
	void VariableDeclaration();
	void TemporaryList();
	void SpecialOperators();
	void InputOperator();
	void OutputOperator();
	void InputArguments();
	void ConditionalSpecialOperators();
	void If();
	void Else();
	void For();
	void While();
	void MultivariateAnalyse(const std::vector<void (Parser::*)()>& variants, bool isCheckEndLineSymbol = false, bool isAssign = false);
	
	bool FunctionExists(string name) {
		return this->declaredFunctions.find(name) != this->declaredFunctions.end();
	}
};

