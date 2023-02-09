#pragma once
#include "Stream.h"
#include "Lexeme.h"
#include <sstream>
#include "Parser.h"

class CompileException : std::exception {
public:
	CompileException() = default;

	CompileException(string str) {
		this->str = str;
	}

	virtual const char* what() {
		return this->str.c_str();
	}

	CompileException(const ReadException& ex) : CompileException(ex.what()) {}

private:
	string str;
};

class CompilationResult {
	bool wasOk;
	CompileException compileException;
	// string outputFile;

public:
	string GetString();
	void SetException(const CompileException& ex);
	CompilationResult();
};

class Compiler
{
	Stream stream;
	Parser* parser;
public:
	vector<LexemeSyntax> GetLexems(string inputFile);

	Compiler() {
		this->parser = nullptr;
	}
	~Compiler() {
		if (this->parser) delete this->parser;
		this->parser = nullptr;
	}

	CompilationResult* Compile(string inputFile);
};

