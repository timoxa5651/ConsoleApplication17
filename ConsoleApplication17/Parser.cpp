#include "Parser.h"
#include "OCompiler.h"
#include "Lexeme.h"

ParserException::ParserException() {
	lexemeType = ELexemeType::Null;
	lexeme = "";
	lexemeNum = 1;
	context = "";
	line = 0;
}

ParserException::ParserException(ELexemeType lexemeType, const std::string& lexeme, int64_t lexemeNum, const std::string& context, int64_t line) {
	this->lexeme = lexeme;
	this->lexemeType = lexemeType;
	this->lexemeNum = lexemeNum;
	this->context = context;
	this->line = line;
}

ParserException::ParserException(const Lexeme& lexeme, int64_t lexemeNum, const std::string& context) {
	this->lexeme = lexeme.string;
	this->lexemeType = lexeme.type;
	this->lexemeNum = lexemeNum;
	this->context = context;
	this->line = lexeme.line;
}

std::string ParserException::what() {
	return "Error in lexeme " + lexeme + "   lexeme type: " + g_LexemeTypeToStr[lexemeType] + "  Context: " + context + "  Line: " + std::to_string(line);
}




Parser::Parser(const vector<LexemeSyntax>& lexemes) {
    std::vector<Lexeme> arr;
    for (auto& lex : lexemes) {
        if (lex.type == ELexemeType::Null) continue;
        arr.emplace_back(lex.type, LexemeTypeDataToStr(lex), lex.line);
    }
    arr.emplace_back(ELexemeType::Null, "End", INT_MAX);
	this->input_ = arr;
	this->currentLexemeIdx = 0;
	this->deepestException.lexemeNum = -2;
    this->MovePtr(0);

	this->currentScope = new FunctionScope(nullptr);
}
Parser::~Parser() {
	if (this->currentScope) {
		this->currentScope->RemoveParent();
		delete this->currentScope;
		this->currentScope = nullptr;
	}
}

void Parser::MovePtr(int idx) {
	this->currentLexemeIdx += idx;
	this->currentLexemeIdx = std::max(this->currentLexemeIdx, 0);
	this->currentLexemeIdx = std::min((size_t)this->currentLexemeIdx, this->input_.size() - 1);
    this->curLexeme_ = this->input_[this->currentLexemeIdx];
}

bool Parser::Check(CompilationResult* result) {
	try {
		//ReadLexeme();
		poliz = Program();
	}
	catch (ParserException& exception) {
		if (exception.lexemeNum < deepestException.lexemeNum) {
			result->SetException(deepestException.what());
		}
		else {
			result->SetException(exception.what());
		}
		return false;
	}
	return true;
}

void Parser::ReadLexeme() {
	this->MovePtr(1);
}

void Parser::ClassStatement() {
    Function();
}

void Parser::ClassBlock() {
	FunctionScope* prevScope = this->currentScope;
	this->currentScope = new FunctionScope(this->currentScope);

    if (curLexeme_.string != "{") {
        throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no opening curly bracket in block definition");
    }
    ReadLexeme();
    while (curLexeme_.string != "}") {
        ClassStatement();
        ReadLexeme();
    }
    if (curLexeme_.string != "}") {
        throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no closing curly bracket in block definition");
    }
	delete this->currentScope;
	this->currentScope = prevScope;
}

Poliz Parser::Program() {
	try {
		bool wasMain = false;
		while (curLexeme_.string == "function" || curLexeme_.string == "class") {

            if (curLexeme_.string == "class") {
                ReadLexeme();
				this->isInAssign = true;
                Name();
				this->isInAssign = false;
				if (!this->declaredClasses.insert(curLexeme_.string).second) {
					throw ParserException(curLexeme_, this->currentLexemeIdx, "Class " + curLexeme_.string + " was already declared");
				}
				this->currentClass = curLexeme_.string;
                ReadLexeme();
                if (curLexeme_.string != "(") {
                    throw ParserException(curLexeme_, this->currentLexemeIdx,
                                          "expected ( in class declaration");
                }
                ReadLexeme();
                Num();
                ReadLexeme();
                if (curLexeme_.string != ")") {
                    throw ParserException(curLexeme_, this->currentLexemeIdx,
                                          "expected ) in class declaration");
                }
                ReadLexeme();
                ClassBlock();
                ReadLexeme();
				this->currentClass = "";
            } else {

                ReadLexeme();
				DeclaredFunction procFunction;
				procFunction.name = curLexeme_.string;
                if (this->FunctionExists(curLexeme_.string))
                    throw ParserException(curLexeme_, this->currentLexemeIdx,
                                          "function " + curLexeme_.string + " declared multiple times");
                poliz.addFunction(curLexeme_.string);
                if (curLexeme_.string == "main") {
                    ReadLexeme();
                    if (curLexeme_.string != "(") {
                        throw ParserException(curLexeme_, this->currentLexemeIdx,
                                              "there is no opening bracket in function declaration");
                    }
                    ReadLexeme();
                    if (curLexeme_.string != ")") {
                        throw ParserException(curLexeme_, this->currentLexemeIdx,
                                              "there is no closing bracket in function declaration");
                    }
                    ReadLexeme();
                    Block();
                    ReadLexeme();
                    wasMain = true;
                    break;
                } else {
                    //ReadLexeme();
                    poliz += Function();
                    ReadLexeme();
                }
            }
		}
		if (curLexeme_.string != "End") {
			//                throw ParserException(curLexeme_, this->currentLexemeIdx, "incorrect function declaration");
			throw ParserException(curLexeme_, this->currentLexemeIdx,
				"incorrect function declaration");
		}
		if (!wasMain) {
			throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no main() in program");
		}
	}
	catch (ParserException& exception) {
		this->isInAssign = false;
		if (exception.lexemeNum < deepestException.lexemeNum && exception.lexemeNum >= 0) {
			throw deepestException;
		}
		else {
			throw exception;
		}
	}
}

Poliz Parser::Function() {
	if (curLexeme_.type != ELexemeType::Variable) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "unnamed function");
	}
	DeclaredFunction func;
	func.name = curLexeme_.string;
	func.numArgs = -1;
	if (!this->currentClass.empty() && this->declaredFunctions[this->currentClass].find(func) != this->declaredFunctions[this->currentClass].end())
		throw ParserException(curLexeme_, this->currentLexemeIdx, "function " + curLexeme_.string + " declared multiple times");
	func.numArgs = 0;
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no opening bracket in function declaration");
	}
	ReadLexeme();
	std::set<string> args;
	if (curLexeme_.string != ")") {
		args = FunctionArgumentsDeclaration();
		func.numArgs = args.size();
		ReadLexeme();
	}
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no closing bracket in function declaration");
	}
	

	this->declaredFunctions[this->currentClass].insert(func);
	FunctionScope* prevScope = this->currentScope;
	this->currentScope = new FunctionScope(this->currentScope);
	for (const string& s : args)
		this->currentScope->InsertVariable(s);
	ReadLexeme();
	Block();

	delete this->currentScope;
	this->currentScope = prevScope;
}

std::set<string> Parser::FunctionArgumentsDeclaration() {
	std::set<string> args;
	this->isInAssign = true;
	Name();
	args.insert(curLexeme_.string);
	this->isInAssign = false;
	ReadLexeme();
	while (curLexeme_.string == ",") {
		ReadLexeme();
		this->isInAssign = true;
		Name();
		if(!args.insert(curLexeme_.string).second)
			throw ParserException(curLexeme_, this->currentLexemeIdx, "Argument declared twice");
		this->isInAssign = false;
		ReadLexeme();
	}
	this->MovePtr(-1);
	return args;
}

Poliz Parser::Block() {
	if (curLexeme_.string != "{") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no opening curly bracket in block definition");
	}
	ReadLexeme();
	FunctionScope* prevScope = this->currentScope;
	this->currentScope = new FunctionScope(this->currentScope);
	while (curLexeme_.string != "}") {
		Statement();
		ReadLexeme();
	}
	delete this->currentScope;
	this->currentScope = prevScope;
	if (curLexeme_.string != "}") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no closing curly bracket in block definition");
	}
}

Poliz Parser::Statement() {
	Exp();
}

Poliz Parser::ValueExp() {
	MultivariateAnalyse({ &Parser::Value, &Parser::Container });
}
Poliz Parser::Operand() {
	MultivariateAnalyse({ &Parser::ListElement, &Parser::FunctionCall, &Parser::Name, &Parser::String, &Parser::Num });
}

Poliz Parser::Value() {
	Priority1();
}

Poliz Parser::Priority1() {
	Priority2();
	ReadLexeme();
	while (curLexeme_.string == "||") {
		ReadLexeme();
		Priority2();
		ReadLexeme();
	}
	this->MovePtr(-1);
}

Poliz Parser::Priority2() {
	Priority3();
	ReadLexeme();
	while (curLexeme_.string == "&&") {
		ReadLexeme();
		Priority3();
		ReadLexeme();
	}
	this->MovePtr(-1);
}

Poliz Parser::Priority3() {
	Priority4();
	ReadLexeme();
	while (curLexeme_.string == "==" || curLexeme_.string == "!=") {
		ReadLexeme();
		Priority4();
		ReadLexeme();
	}
	this->MovePtr(-1);
}

Poliz Parser::Priority4() {
	Priority5();
	ReadLexeme();
	while (curLexeme_.string == ">=" || curLexeme_.string == "<=" || curLexeme_.string == "<" || curLexeme_.string == ">") {
		ReadLexeme();
		Priority5();
		ReadLexeme();
	}
	this->MovePtr(-1);
}

Poliz Parser::Priority5() {
	Priority6();
	ReadLexeme();
	while (curLexeme_.string == "+" || curLexeme_.string == "-") {
		ReadLexeme();
		Priority6();
		ReadLexeme();
	}
	this->MovePtr(-1);
}

Poliz Parser::Priority6() {
	Priority7();
	ReadLexeme();
	while (curLexeme_.string == "*" || curLexeme_.string == "/" || curLexeme_.string == "//" || curLexeme_.string == "%") {
		ReadLexeme();
		Priority7();
		ReadLexeme();
	}
	this->MovePtr(-1);
}

Poliz Parser::Priority7() {
	if (curLexeme_.string == "-" || curLexeme_.string == "!") {
		ReadLexeme();
		Priority8();
	}
	else {
		Priority8();
	}
}

Poliz Parser::Priority8() {
	if (curLexeme_.string == "(") {
		ReadLexeme();
		Priority1();
		ReadLexeme();
		if (curLexeme_.string != ")") {
			throw ParserException(curLexeme_, this->currentLexemeIdx, "missed closing bracket");
		}
	}
	else {
		Operand();
	}
}


Poliz Parser::FunctionCall() {
	this->isInFuncCall = true;
	try {
		Name();
	}
	catch (const ParserException& ex) {
		this->isInFuncCall = false;
		throw;
	}
	this->isInFuncCall = false;
	DeclaredFunction currentFunc;
	currentFunc.name = this->lastReadName;
	currentFunc.numArgs = -1;
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx - 1, "missed opening bracket in function call");
	}
	ReadLexeme();
	int passedParams = 0;
	if (curLexeme_.string != ")") {
		passedParams = Arguments();
	}
	else {
		return Poliz();
	}
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "missed closing bracket in function call");
	}

	const DeclaredFunction& dFunc = *this->declaredFunctions[this->currentClass].find(currentFunc);
	if (dFunc.numArgs != passedParams) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid arguments " + std::to_string(passedParams) + " != " + std::to_string(dFunc.numArgs));
	}
}

int Parser::Arguments() {
	int numArgs = 1;
	ValueExp();
	ReadLexeme();
	while (curLexeme_.string == ",") {
		ReadLexeme();
		ValueExp();
		ReadLexeme();
		numArgs += 1;
	}
	this->MovePtr(-1);
	return numArgs;
}

Poliz Parser::Container() {
	MultivariateAnalyse({&Parser::FunctionCall, &Parser::List, &Parser::String });
}

Poliz Parser::String() {
	if (curLexeme_.type != ELexemeType::LiteralStr && curLexeme_.type != ELexemeType::LiteralChar) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid string literal");
	}
}

Poliz Parser::Name() {
	if (curLexeme_.type == ELexemeType::Keyword) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "variable can not be named by keyword");
	}
	if (curLexeme_.type != ELexemeType::Variable) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid variable name");
	}
	DeclaredFunction func;
	func.name = curLexeme_.string;
	func.numArgs = -1;
	bool isFunction = this->declaredFunctions[this->currentClass].find(func) != this->declaredFunctions[this->currentClass].end();
	if (!this->isInFuncCall) {
		if (isFunction) {
			throw ParserException(curLexeme_, this->currentLexemeIdx, "unable to reference function");
		}
	}

	if (!isFunction && !this->isInAssign && !this->currentScope->HasVariable(curLexeme_.string)) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "undefined variable");
	}
	this->lastReadName = curLexeme_.string;
//    if (isInFuncCall) {
//        poliz.addEntryToBlock(PolizCmd::Call, curLexeme_.string);
//    } else {
//        poliz.addEntryToBlock(PolizCmd::Var, curLexeme_.string);
//    }
}

Poliz Parser::Num() {
	if (curLexeme_.type != ELexemeType::LiteralDouble && curLexeme_.type != ELexemeType::LiteralNum32 && curLexeme_.type != ELexemeType::LiteralNum64) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid number");
	}
//    poliz.addEntryToBlock(PolizCmd::Const, curLexeme_.string);
}

Poliz Parser::List() {
	MultivariateAnalyse({ &Parser::TemporaryList, &Parser::Name });
}

Poliz Parser::ListElement() {
	Name();
//    poliz.changeEntryCmdInBlock(poliz.GetCurAddress(), PolizCmd::ArrayAccess);
	ReadLexeme();
	if (curLexeme_.string != "[") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening box bracket in getting list element");
	}
	ReadLexeme();
	MultivariateAnalyse({ &Parser::Name, &Parser::Num, &Parser::ListElement });
	ReadLexeme();
	if (curLexeme_.string != "]") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing box bracket in getting list element");
	}
}

Poliz Parser::Exp() {
	MultivariateAnalyse({/*&Parser::VariableDeclaration, &Parser::ListDeclaration,*/ &Parser::FunctionCall,
						 &Parser::SpecialOperators, &Parser::ConditionalSpecialOperators, &Parser::Assign, &Parser::Return }, true);
}

Poliz Parser::Return() {
	if (curLexeme_.string != "return") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid return operator");
	}
	ReadLexeme();
	if (curLexeme_.string != ";") {
		ValueExp();
	}
	else {
		this->MovePtr(-1);
	}
}

Poliz Parser::Assign() {
	MultivariateAnalyse({ &Parser::ListElement, &Parser::Name }, false, true);
	string newVar = "";
	bool flag = false;
	if (curLexeme_.type == ELexemeType::Variable) {
		newVar = curLexeme_.string;
		flag = true;
	}
	ReadLexeme();
	if (curLexeme_.string != "=") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid assignment operator");
	}

//    poliz.addEntryBlockToPoliz();

	ReadLexeme();
	ValueExp();

//    poliz.addEntryToBlock(PolizCmd::Operation, "=");
	if (flag) {
		this->currentScope->InsertVariable(newVar);
	}
}

void Parser::VariableDeclaration() {
	if (curLexeme_.string != "var") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected keyword var");
	}
	ReadLexeme();
	Name();
	ReadLexeme();
	if (curLexeme_.string == "=") {
		ReadLexeme();
		ValueExp();
	}
	else {
		this->MovePtr(-1);
	}

}

Poliz Parser::TemporaryList() {
	if (curLexeme_.string != "[") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening box bracket in list declaration");
	}
	ReadLexeme();
	if (curLexeme_.string != "]") {
		Arguments();
		ReadLexeme();
	}
	if (curLexeme_.string != "]") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing box bracket in list declaration");
	}
}

Poliz Parser::SpecialOperators() {
	MultivariateAnalyse({ &Parser::InputOperator, &Parser::OutputOperator });
}

Poliz Parser::InputOperator() {
	if (curLexeme_.string != "read") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid input operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in function call");
	}
	ReadLexeme();
	InputArguments();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in function call");
	}
}

Poliz Parser::OutputOperator() {
	if (curLexeme_.string != "print") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid output operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in function call");
	}
	ReadLexeme();
	Arguments();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in function call");
	}
}

Poliz Parser::InputArguments() {
	MultivariateAnalyse({ &Parser::Name, &Parser::ListElement });
}

Poliz Parser::ConditionalSpecialOperators() {
	MultivariateAnalyse({ &Parser::For, &Parser::While, &Parser::If });
}

Poliz Parser::If() {
	if (curLexeme_.string != "if") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid if operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in if structure");
	}
	ReadLexeme();
	ValueExp();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in if structure");
	}
	ReadLexeme();
	Block();
	ReadLexeme();
	if (curLexeme_.string == "else") {
		Else();
	}
	else {
		this->MovePtr(-1);
	}
}

Poliz Parser::Else() {
	if (curLexeme_.string != "else") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid else operator");
	}
	ReadLexeme();
	MultivariateAnalyse({ &Parser::Block, &Parser::If });
}

Poliz Parser::For() {
	if (curLexeme_.string != "for") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid for operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in for structure");
	}

	ReadLexeme();
	this->isInAssign = true;
	Name();
	this->isInAssign = false;
	ReadLexeme();
	if (curLexeme_.string != "in") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected keyword IN");
	}
	ReadLexeme();
	Container();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in for structure");
	}
	ReadLexeme();
	Block();
}

Poliz Parser::While() {
	if (curLexeme_.string != "while") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid while operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in while structure");
	}
	ReadLexeme();
	ValueExp();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in while structure");
	}
	ReadLexeme();
	Block();
}

Poliz Parser::MultivariateAnalyse(const std::vector<Poliz (Parser::*)()>& variants, bool isCheckEndLineSymbol, bool isAssign) {
	int64_t pos = (int64_t)this->currentLexemeIdx - 1;
	bool flag = true;
	ParserException exception_(ELexemeType::Null, "", -1, "", 0);
	for (auto fun : variants) {
		try {
			this->MovePtr(pos - this->currentLexemeIdx);
			ReadLexeme();
			if (fun == &Parser::Name) {
				this->isInAssign = isAssign;
			}
			(this->*fun)();
			if (isCheckEndLineSymbol && fun != &Parser::ConditionalSpecialOperators) {
				ReadLexeme();
				if (curLexeme_.string != ";") {
					throw ParserException(curLexeme_, this->currentLexemeIdx, "expected end of line symbol");
				}
			}
		}
		catch (ParserException& exception) {
			if (exception.lexemeNum > exception_.lexemeNum) {
				exception_ = exception;
				if (exception.lexemeNum > deepestException.lexemeNum) {
					deepestException = exception;
				}
			}
//            poliz.clearEntryBlock();
			continue;
		}
		flag = false;
//        poliz.addEntryBlockToPoliz();
		break;
	}
	this->isInAssign = false;
	if (flag) {
		throw exception_;
	}

}


