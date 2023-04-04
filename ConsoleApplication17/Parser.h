#pragma once
#include <string>
#include <vector>
#include <exception>
#include <set>
#include "Lexeme.h"
#include "Stream.h"
#include "Poliz.h"

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

	bool operator==(const DeclaredFunction& ex) const {
		return ex.name == this->name && (ex.numArgs < 0 || this->numArgs < 0 || this->numArgs == ex.numArgs);
	}
	bool operator<(const DeclaredFunction& ex) const {
		return this->name < ex.name;
	}
	DeclaredFunction() {
		this->numArgs = 0;
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
	string lastReadName;
    Poliz poliz;

	void MovePtr(int idx);

    void ClassBlock();
    void ClassStatement();

	void ReadLexeme();

	Poliz Program();
    Poliz Function();

	std::set<string> FunctionArgumentsDeclaration();

    Poliz Block();
    Poliz Statement();
    Poliz ValueExp();
    Poliz Operand();
    Poliz Value();
    Poliz Priority1();
    Poliz Priority2();
    Poliz Priority3();
    Poliz Priority4();
    Poliz Priority5();
    Poliz Priority6();
    Poliz Priority7();
    Poliz Priority8();
    Poliz FunctionCall();

	std::pair<int, Poliz> Arguments();

    Poliz Container();
    Poliz String();
    Poliz Name();
    Poliz Num();
    Poliz List();
    Poliz ListElement();
    Poliz Exp();
    Poliz Return();
    Poliz Assign();
	void VariableDeclaration();
    Poliz TemporaryList();
    Poliz SpecialOperators();
    Poliz InputOperator();
    Poliz OutputOperator();
    Poliz InputArguments();
    Poliz ConditionalSpecialOperators();
    Poliz If();
    Poliz Else();
    Poliz For();
    Poliz While();
    Poliz MultivariateAnalyse(const std::vector<Poliz (Parser::*)()>& variants, bool isCheckEndLineSymbol = false, bool isAssign = false);
	
	bool FunctionExists(string name) {
		return this->declaredFunctions.find(name) != this->declaredFunctions.end();
	}
};

