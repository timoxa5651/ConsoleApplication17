#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <exception>
#include <fstream>
#include <sstream>
#include "climits"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::exception;

using __int64 = int64_t;

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

    LexemeSyntax(ELexemeType t, int line, int pos) {
        this->type = t;
        this->line = line;
        this->pos = pos;
    };

    template<typename T>
    LexemeSyntax(ELexemeType t, const T& value, int line, int pos) : LexemeSyntax(t, line, pos) {
        this->set_val<T>(value);
    };

    ~LexemeSyntax() {}
};


static string LexemeTypeDataToStr(LexemeSyntax lexeme) {
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
                rd = std::max((int64_t) 1, rd * 10);
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


class Lexeme{
public:
    ELexemeType type;
    std::string string;
    int64_t line;

    Lexeme() {
        type = ELexemeType::Null;
        string = "";
        line = 0;
    }

    Lexeme(ELexemeType lexemeType, std::string string, int64_t line) {
        this->type = lexemeType;
        this->string = std::move(string);
        this->line = line;
    }

    int64_t GetLineLength() {
        return string.size() + g_LexemeTypeToStr[type].size() + 2 + std::to_string(line).size();
    }
};


class  ParserException : std::exception {
public:
    ELexemeType lexemeType;
    std::string lexeme;
    int64_t lexemeNum;
    std::string context;
    int64_t line;

    ParserException() {
        lexemeType = ELexemeType::Null;
        lexeme = "";
        lexemeNum = 1;
        context = "";
        line = 0;
    }

    ParserException(ELexemeType lexemeType, const std::string& lexeme, int64_t lexemeNum, const std::string& context, int64_t line) {
        this->lexeme = lexeme;
        this->lexemeType = lexemeType;
        this->lexemeNum = lexemeNum;
        this->context = context;
        this->line = line;
    }

    ParserException(const Lexeme& lexeme, int64_t lexemeNum, const std::string& context) {
        this->lexeme = lexeme.string;
        this->lexemeType = lexeme.type;
        this->lexemeNum =  lexemeNum;
        this->context = context;
        this->line = lexeme.line;
    }

    virtual std::string what() {
        return "Error in lexeme " + lexeme + "   lexeme type: " + g_LexemeTypeToStr[lexemeType] + "  Context: " + context + "  Line: " + std::to_string(line);
    }
};


class Parser{
public:
    explicit Parser(const std::string& filePath) {
        input_ = std::ifstream(filePath);
        deepestException.lexemeNum = -2;
    }

    void Check() {
        try {
            ReadLexeme();
            Program();
//            ReadLexeme();
//            if (curLexeme_.string != "End") {
//                throw ParserException(curLexeme_, input_.tellg(), "there are unwanted symbols after main()");
//            }
        } catch (ParserException& exception) {
            if (exception.lexemeNum < deepestException.lexemeNum) {
                std::cout << deepestException.what();
            } else {
                std::cout << exception.what() << '\n';
            }
            return;
        }
    }
private:
    std::ifstream input_;
    Lexeme curLexeme_;
    ParserException deepestException;

    void ReadLexeme() {
        std::string type = "Null";
        while (type == "Null" && !input_.eof() && type != "End") {
            input_ >> type;
            if (type == "Null" || type == "End") continue;
            input_ >> curLexeme_.string;
            input_ >> curLexeme_.line;
        }
        if (!input_) return;

        if (type == "Null") {
            curLexeme_.type = ELexemeType::Null;
        } else if (type == "Invalid") {
            curLexeme_.type = ELexemeType::Invalid;
        } else if (type == "Variable") {
            curLexeme_.type = ELexemeType::Variable;
        } else if (type == "Keyword") {
            curLexeme_.type = ELexemeType::Keyword;
        } else if (type == "LiteralStr") {
            curLexeme_.type = ELexemeType::LiteralStr;
        } else if (type == "Operator") {
            curLexeme_.type = ELexemeType::Operator;
        } else if (type == "LiteralChar") {
            curLexeme_.type = ELexemeType::LiteralChar;
        } else if (type == "RoundBrack") {
            curLexeme_.type = ELexemeType::RoundBrack;
        } else if (type == "BoxBrack") {
            curLexeme_.type = ELexemeType::BoxBrack;
        } else if (type == "CurlyBrack") {
            curLexeme_.type = ELexemeType::CurlyBrack;
        } else if (type == "Punctuation") {
            curLexeme_.type = ELexemeType::Punctuation;
        } else if (type == "LiteralNum32") {
            curLexeme_.type = ELexemeType::LiteralNum32;
        } else if (type == "LiteralNum64") {
            curLexeme_.type = ELexemeType::LiteralNum64;
        } else if (type == "LiteralDouble") {
            curLexeme_.type = ELexemeType::LiteralDouble;
        } else if (type == "End") {
            curLexeme_.string = type;
        }
    }

    void Program() {
        try {
            bool wasMain = false;
            while (curLexeme_.string == "function") {
                ReadLexeme();
                if (curLexeme_.string == "main") {
                    ReadLexeme();
                    if (curLexeme_.string != "(") {
                        //throw ParserException(curLexeme_, input_.tellg(), "there is no opening bracket in function declaration", curLexeme_.line);
                        throw ParserException(curLexeme_, input_.tellg(),
                                              "there is no opening bracket in function declaration");
                    }
                    ReadLexeme();
                    if (curLexeme_.string != ")") {
                        //throw ParserException(curLexeme_, input_.tellg(), "there is no closing bracket in function declaration");
                        throw ParserException(curLexeme_, input_.tellg(),
                                              "there is no closing bracket in function declaration");
                    }
                    ReadLexeme();
                    Block();
                    ReadLexeme();
                    wasMain = true;
                    break;
                } else {
                    //ReadLexeme();
                    Function();
                    ReadLexeme();
                }
            }
            if (curLexeme_.string != "End") {
//                throw ParserException(curLexeme_, input_.tellg(), "incorrect function declaration");
                throw ParserException(curLexeme_, input_.tellg(),
                                      "incorrect function declaration");
            }
            if (!wasMain) {
                throw ParserException(curLexeme_, input_.tellg(), "there is no main() in program");
            }

//            if (curLexeme_.string == "function") {
//                ReadLexeme();
//                if (curLexeme_.string == "main") {
//                    ReadLexeme();
//                    if (curLexeme_.string != "(") {
//                        throw ParserException(curLexeme_, input_.tellg(), "there is no opening bracket in function declaration");
//                    }
//                    ReadLexeme();
//                    if (curLexeme_.string != ")") {
//                        throw ParserException(curLexeme_, input_.tellg(), "there is no closing bracket in function declaration");
//                    }
//                    ReadLexeme();
//                    Block();
//                } else {
//                    //ReadLexeme();
//                    Function();
//                    ReadLexeme();
//                    Program();
//                }
//            } else {
//                throw ParserException(curLexeme_, input_.tellg(), "there is no keyword in function declaration");
//            }
        } catch (ParserException& exception) {
            if (exception.lexemeNum < deepestException.lexemeNum && exception.lexemeNum >= 0) {
                std::cout << deepestException.what() << '\n';
            } else {
                std::cout << exception.what() << '\n';
            }
        }
    }

    void Function() {
        if (curLexeme_.type != ELexemeType::Variable) {
            throw ParserException(curLexeme_, input_.tellg(), "unnamed function");
        }
        ReadLexeme();
        if (curLexeme_.string != "(") {
            throw ParserException(curLexeme_, input_.tellg(), "there is no opening bracket in function declaration");
        }
        ReadLexeme();
        if (curLexeme_.string != ")") {
            FunctionArgumentsDeclaration();
            ReadLexeme();
        }
        if (curLexeme_.string != ")") {
            throw ParserException(curLexeme_, input_.tellg(), "there is no closing bracket in function declaration");
        }
        ReadLexeme();
//        RootBlock();
        Block();
    }

    void FunctionArgumentsDeclaration() {
        Name();
        ReadLexeme();
        while (curLexeme_.string == ",") {
            ReadLexeme();
            Name();
            ReadLexeme();
        }
        int64_t pos = input_.tellg();
        input_.seekg(pos - curLexeme_.GetLineLength());
    }

//    void RootBlock() {
//        if (curLexeme_.type != ELexemeType::CurlyBrack || curLexeme_.string != "{") {
//            throw ParserException(curLexeme_, input_.tellg(), "there is no opening curly bracket in function declaration");
//        }
//        ReadLexeme();
//        while (curLexeme_.string != "}" /*&& curLexeme_.string != "return"*/) {
//            Statement();
//            ReadLexeme();
//        }
////        if (curLexeme_.string == "return") {
////            ReadLexeme();
////            if (curLexeme_.string != ";") {
////                //Name();
////                ValueExp();
////                ReadLexeme();
////            }
////            if (curLexeme_.string != ";") {
////                throw ParserException(curLexeme_, input_.tellg(), "there is no end of line symbol");
////            }
////            ReadLexeme();
////        }
//        if (curLexeme_.string != "}") {
//            throw ParserException(curLexeme_, input_.tellg(), "there is no opening curly bracket in function declaration");
//        }
//    }

    void Block() {
        if (curLexeme_.string != "{") {
            throw ParserException(curLexeme_, input_.tellg(), "there is no opening curly bracket in block definition");
        }
        ReadLexeme();
//        if (curLexeme_.string == "return") throw ParserException(curLexeme_, input_.tellg());
        while (curLexeme_.string != "}") {
            Statement();
            ReadLexeme();
        }
        if (curLexeme_.string != "}") {
            throw ParserException(curLexeme_, input_.tellg(), "there is no closing curly bracket in block definition");
        }
    }

    void Statement() {
//        int64_t pos = (int64_t)input_.tellg() - curLexeme_.GetLineLength();
//        try {
//            ValueExp();
//            ReadLexeme();
//            if (curLexeme_.string != ";") {
//                throw ParserException(curLexeme_, input_.tellg());
//            }
//        } catch (ParserException& exception) {
//            input_.seekg(pos);
//            ReadLexeme();
//            Exp();
//        }
//        ReadLexeme();
//        if (curLexeme_.string != ";") {
//            throw ParserException(curLexeme_, input_.tellg());
//        }
        Exp();
//        ReadLexeme();
//        if (curLexeme_.string != ";") {
//            throw ParserException(curLexeme_, input_.tellg());
//        }
    }

    void ValueExp() {
        MultivariateAnalyse({&Parser::Value, &Parser::Container});
    }
    void Operand() {
        MultivariateAnalyse({&Parser::ListElement, &Parser::FunctionCall, &Parser::Name, &Parser::String, &Parser::Num});
    }

    void Value() {
        Priority1();
    }

    void Priority1() {
        Priority2();
        ReadLexeme();
        while (curLexeme_.string == "||") {
            ReadLexeme();
            Priority2();
            ReadLexeme();
        }
        int64_t pos = input_.tellg();
        input_.seekg(pos - curLexeme_.GetLineLength());
    }

    //void Operation1() {}

    void Priority2() {
        Priority3();
        ReadLexeme();
        while (curLexeme_.string == "&&") {
            ReadLexeme();
            Priority3();
            ReadLexeme();
        }
        int64_t pos = input_.tellg();
        input_.seekg(pos - curLexeme_.GetLineLength());
    }

    //void Operation2() {}

    void Priority3() {
        Priority4();
        ReadLexeme();
        while (curLexeme_.string == "==" || curLexeme_.string == "!=") {
            ReadLexeme();
            Priority4();
            ReadLexeme();
        }
        int64_t pos = input_.tellg();
        input_.seekg(pos - curLexeme_.GetLineLength());
    }

    //void Operation3() {}

    void Priority4() {
        Priority5();
        ReadLexeme();
        while (curLexeme_.string == ">=" || curLexeme_.string == "<=" || curLexeme_.string == "<" || curLexeme_.string == ">") {
            ReadLexeme();
            Priority5();
            ReadLexeme();
        }
        int64_t pos = input_.tellg();
        input_.seekg(pos - curLexeme_.GetLineLength());
    }

    //void Operation4() {}

    void Priority5() {
        Priority6();
        ReadLexeme();
        while (curLexeme_.string == "+" || curLexeme_.string == "-") {
            ReadLexeme();
            Priority6();
            ReadLexeme();
        }
        int64_t pos = input_.tellg();
        input_.seekg(pos - curLexeme_.GetLineLength());
    }

    //void Operation5() {}

    void Priority6() {
        Priority7();
        ReadLexeme();
        while (curLexeme_.string == "*" || curLexeme_.string == "/" || curLexeme_.string == "//" || curLexeme_.string == "%") {
            ReadLexeme();
            Priority7();
            ReadLexeme();
        }
        int64_t pos = input_.tellg();
        input_.seekg(pos - curLexeme_.GetLineLength());
    }

    //void Operation6() {}

    void Priority7() {
        //ReadLexeme();
        if (curLexeme_.string == "-" || curLexeme_.string == "!") {
            ReadLexeme();
            Priority8();
        } else {
            Priority8();
        }
    }

    //void Operation7() {}

    void Priority8() {
        if (curLexeme_.string == "(") {
            ReadLexeme();
            Priority1();
            ReadLexeme();
            if (curLexeme_.string != ")") {
                throw ParserException(curLexeme_, input_.tellg(), "missed closing bracket");
            }
        } else {
            Operand();
        }
    }


    void FunctionCall() {
        Name();
        ReadLexeme();
        if (curLexeme_.string != "(") {
            throw ParserException(curLexeme_, input_.tellg(), "missed opening bracket in function call");
        }
        ReadLexeme();
        if (curLexeme_.string != ")") {
            Arguments();
        } else {
            return;
        }
        ReadLexeme();
        if (curLexeme_.string != ")") {
            throw ParserException(curLexeme_, input_.tellg(), "missed closing bracket in function call");
        }
    }

    void Arguments() {
//        Operand();
//        ReadLexeme();
//        while (curLexeme_.string == ",") {
//            ReadLexeme();
//            Operand();
//            ReadLexeme();
//        }
//        int64_t pos = input_.tellg();
//        input_.seekg(pos - curLexeme_.GetLineLength());
        ValueExp();
        ReadLexeme();
        while (curLexeme_.string == ",") {
            ReadLexeme();
            ValueExp();
            ReadLexeme();
        }
        int64_t pos = input_.tellg();
        input_.seekg(pos - curLexeme_.GetLineLength());
    }

    void Container() {
        MultivariateAnalyse({/*&Parser::Range*/ &Parser::FunctionCall, &Parser::List, &Parser::String});
    }

    void String() {
        if (curLexeme_.type != ELexemeType::LiteralStr && curLexeme_.type != ELexemeType::LiteralChar) {
            throw ParserException(curLexeme_, input_.tellg(), "invalid string literal");
        }
//        if (curLexeme_.string != "\"") {
//            throw ParserException(curLexeme_, input_.tellg());
//        }
//        ReadLexeme();
//        for(auto item : curLexeme_.string) {
//            item = std::tolower(item);
//            if ((item < '0' || item > '9') && (item < 'a' || item > 'z') && (item < 'A' || item > 'Z')) {
//                throw ParserException(curLexeme_, input_.tellg());
//            }
//        }
//        ReadLexeme();
//        if (curLexeme_.string != "\"") {
//            throw ParserException(curLexeme_, input_.tellg());
//        }
    }

    void Name() {
        if (curLexeme_.type == ELexemeType::Keyword) {
            throw ParserException(curLexeme_, input_.tellg(), "variable can not be named by keyword");
        }
        if (curLexeme_.type != ELexemeType::Variable) {
            throw ParserException(curLexeme_, input_.tellg(), "invalid variable name");
        }
//        if ('a' > tolower(curLexeme_.string[0]) || tolower(curLexeme_.string[0]) > 'z') {
//            throw ParserException(curLexeme_, input_.tellg());
//        }
//        for (int i = 1; i < curLexeme_.string.size(); ++i) {
//            if (('a' > tolower(curLexeme_.string[i]) || tolower(curLexeme_.string[i]) > 'z') && (curLexeme_.string[i] < '0' || curLexeme_.string[i] > '9')) {
//                throw ParserException(curLexeme_, input_.tellg());
//            }
//        }
    }

    //void Symbol() {}
    void Num() {
        if (curLexeme_.type != ELexemeType::LiteralDouble && curLexeme_.type != ELexemeType::LiteralNum32 && curLexeme_.type != ELexemeType::LiteralNum64) {
            throw ParserException(curLexeme_, input_.tellg(), "invalid number");
        }
    }

    void List() {
        MultivariateAnalyse({&Parser::TemporaryList, &Parser::Name});
//        Name();
    }

    void ListElement() {
        Name();
        ReadLexeme();
        if (curLexeme_.string != "[") {
            throw ParserException(curLexeme_, input_.tellg(), "expected opening box bracket in getting list element");
        }
        ReadLexeme();
        MultivariateAnalyse({&Parser::Name, &Parser::Num, &Parser::ListElement});
        ReadLexeme();
        if (curLexeme_.string != "]") {
            throw ParserException(curLexeme_, input_.tellg(), "expected closing box bracket in getting list element");
        }
    }

    void Exp() {
        MultivariateAnalyse({/*&Parser::VariableDeclaration, &Parser::ListDeclaration,*/ &Parser::FunctionCall,
                             &Parser::SpecialOperators, &Parser::ConditionalSpecialOperators, &Parser::Assign, &Parser::Return}, true);
    }

    void Return() {
        if (curLexeme_.string != "return") {
            throw ParserException(curLexeme_, input_.tellg(), "invalid return operator");
        }
        ReadLexeme();
        if (curLexeme_.string != ";") {
            ValueExp();
//            Name();
        } else {
            input_.seekg(input_.tellg() - curLexeme_.GetLineLength());
        }
//        ReadLexeme();
//        if (curLexeme_.string != ";") {
//            throw ParserException(curLexeme_, input_.tellg(), "there is no end of line symbol");
//        }
    }

    void Assign() {
        //Name();
        MultivariateAnalyse({&Parser::ListElement, &Parser::Name});
        ReadLexeme();
        if(curLexeme_.string != "=") {
            throw ParserException(curLexeme_, input_.tellg(), "invalid assignment operator");
        }
        ReadLexeme();
        ValueExp();
    }

    void VariableDeclaration() {
        if (curLexeme_.string != "var") {
            throw ParserException(curLexeme_, input_.tellg(), "expected keyword var");
        }
        ReadLexeme();
        Name();
        ReadLexeme();
        if (curLexeme_.string == "=") {
            ReadLexeme();
            ValueExp();
        } else {
            int64_t pos = input_.tellg();
            input_.seekg(pos - curLexeme_.GetLineLength());
        }

    }

//    void ListDeclaration() {
//        if (curLexeme_.string != "var") {
//            throw ParserException(curLexeme_, input_.tellg(), "expected keyword var");
//        }
//        ReadLexeme();
//        Name();
//        ReadLexeme();
//        if (curLexeme_.string != "=") {
//            throw ParserException(curLexeme_, input_.tellg(), "invalid assignment operator");
//        }
//        ReadLexeme();
//        TemporaryList();
////        if (curLexeme_.string != "[") {
////            throw ParserException(curLexeme_, input_.tellg(), "expected opening box bracket in list declaration");
////        }
////        ReadLexeme();
////        Arguments();
////        ReadLexeme();
////        if (curLexeme_.string != "]") {
////            throw ParserException(curLexeme_, input_.tellg(), "expected closing box bracket in list declaration");
////        }
//    }

    void TemporaryList() {
        if (curLexeme_.string != "[") {
            throw ParserException(curLexeme_, input_.tellg(), "expected opening box bracket in list declaration");
        }
        ReadLexeme();
        if (curLexeme_.string != "]") {
            Arguments();
            ReadLexeme();
        }
        if (curLexeme_.string != "]") {
            throw ParserException(curLexeme_, input_.tellg(), "expected closing box bracket in list declaration");
        }
    }

    void SpecialOperators() {
        MultivariateAnalyse({&Parser::InputOperator, &Parser::OutputOperator});
    }

    void InputOperator() {
        if (curLexeme_.string != "read") {
            throw ParserException(curLexeme_, input_.tellg(), "invalid input operator");
        }
        ReadLexeme();
        if (curLexeme_.string != "(") {
            throw ParserException(curLexeme_, input_.tellg(), "expected opening bracket in function call");
        }
        ReadLexeme();
        InputArguments();
        ReadLexeme();
        if (curLexeme_.string != ")") {
            throw ParserException(curLexeme_, input_.tellg(), "expected closing bracket in function call");
        }
    }

    void OutputOperator() {
        if (curLexeme_.string != "print") {
            throw ParserException(curLexeme_, input_.tellg(), "invalid output operator");
        }
        ReadLexeme();
        if (curLexeme_.string != "(") {
            throw ParserException(curLexeme_, input_.tellg(), "expected opening bracket in function call");
        }
        ReadLexeme();
        Arguments();
        ReadLexeme();
        if (curLexeme_.string != ")") {
            throw ParserException(curLexeme_, input_.tellg(), "expected closing bracket in function call");
        }
    }

    void InputArguments() {
        MultivariateAnalyse({&Parser::Name, &Parser::ListElement});
    }
    //void Return() {}

    void ConditionalSpecialOperators() {
        MultivariateAnalyse({&Parser::For, &Parser::While, &Parser::If});
    }

    void If() {
        if (curLexeme_.string != "if") {
            throw ParserException(curLexeme_, input_.tellg(), "invalid if operator");
        }
        ReadLexeme();
        if (curLexeme_.string != "(") {
            throw ParserException(curLexeme_, input_.tellg(), "expected opening bracket in if structure");
        }
        ReadLexeme();
        ValueExp();
        ReadLexeme();
        if (curLexeme_.string != ")") {
            throw ParserException(curLexeme_, input_.tellg(), "expected closing bracket in if structure");
        }
        ReadLexeme();
        Block();
        ReadLexeme();
        if (curLexeme_.string == "else") {
            Else();
        } else {
            int64_t pos = input_.tellg();
            input_.seekg(pos - curLexeme_.GetLineLength());
        }
    }

    void Else() {
        if (curLexeme_.string != "else") {
            throw ParserException(curLexeme_, input_.tellg(), "invalid else operator");
        }
        ReadLexeme();
        MultivariateAnalyse({&Parser::Block, &Parser::If});
    }

    void For() {
        if (curLexeme_.string != "for") {
            throw ParserException(curLexeme_, input_.tellg(), "invalid for operator");
        }
        ReadLexeme();
        if (curLexeme_.string != "(") {
            throw ParserException(curLexeme_, input_.tellg(), "expected opening bracket in for structure");
        }
//        ReadLexeme();
//        if (curLexeme_.string != "var") {
//            throw ParserException(curLexeme_, input_.tellg(), "expected iterator declaration");
//        }
        ReadLexeme();
        Name();
        ReadLexeme();
        if (curLexeme_.string != "in"){
            throw ParserException(curLexeme_, input_.tellg(), "expected keyword IN");
        }
        ReadLexeme();
        Container();
        ReadLexeme();
        if (curLexeme_.string != ")") {
            throw ParserException(curLexeme_, input_.tellg(), "expected closing bracket in for structure");
        }
        ReadLexeme();
        Block();
    }

    void While() {
        if (curLexeme_.string != "while") {
            throw ParserException(curLexeme_, input_.tellg(), "invalid while operator");
        }
        ReadLexeme();
        if (curLexeme_.string != "(") {
            throw ParserException(curLexeme_, input_.tellg(), "expected opening bracket in while structure");
        }
        ReadLexeme();
        ValueExp();
        ReadLexeme();
        if (curLexeme_.string != ")") {
            throw ParserException(curLexeme_, input_.tellg(), "expected closing bracket in while structure");
        }
        ReadLexeme();
        Block();
    }

    void MultivariateAnalyse(const std::vector<void (Parser::*)()>& variants, bool isCheckEndLineSymbol = false) {
        int64_t pos = (int64_t)input_.tellg() - curLexeme_.GetLineLength();
        bool flag = true;
        ParserException exception_(ELexemeType::Null, "", -1, "", 0);
        for(auto fun : variants) {
            try {
                input_.seekg(pos);
                ReadLexeme();
                (this->*fun)();
                if (isCheckEndLineSymbol && fun != &Parser::ConditionalSpecialOperators) {
                    ReadLexeme();
                    if (curLexeme_.string != ";") {
                        throw ParserException(curLexeme_, input_.tellg(), "expected end of line symbol");
                    }
                }
            } catch (ParserException& exception) {
                if (exception.lexemeNum > exception_.lexemeNum) {
                    exception_ = exception;
                    if (exception.lexemeNum > deepestException.lexemeNum) {
                        deepestException = exception;
                    }
                }
                continue;
            }
            flag = false;
            break;
        }
        if (flag) {
            throw exception_;
            //throw ParserException(exception_.lexemeType, exception_.lexeme, exception_.lexemeNum, exception_.context);
            //throw ParserException(curLexeme_, input_.tellg());
        }

    }
};


int main()
{
    vector<string> resvKeywords/* = { "var", "while", "for", "if", "switch" }*/;
    vector<string> resvOperators = { "+", "*", "/", "-", "%", "=", "&&", "==", "!=", "||", "<", "<=", ">=", ">", "//"};

    std::ifstream keyWordsFile("../keyWordsFile.txt");
    string str;
    while(keyWordsFile >> str) {
        resvKeywords.push_back(str);
    }
    std::ifstream t("../input.txt");
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string input = buffer.str();

    vector<LexemeSyntax> result;
    result.reserve(100);

    int currentLine = 1;
    int lineStartPos = 0;

    Stream stream = input;
    while (!stream.is_end()) {
        char rd = stream.peek_char();
        if (rd == ' ' || rd == '\n' || rd == '\t') {
            if (rd == '\n') {
                currentLine += 1;
                lineStartPos = stream.get_cur();
            }
            stream.seek(1);
            result.push_back(LexemeSyntax(ELexemeType::Null, currentLine, stream.get_cur() - lineStartPos));
            continue;
        }
        else if (rd == '{' || rd == '}') {
            stream.seek(1);
            result.push_back(LexemeSyntax(ELexemeType::CurlyBrack, rd, currentLine, stream.get_cur() - lineStartPos));
            continue;
        }
        else if (rd == '[' || rd == ']') {
            stream.seek(1);
            result.push_back(LexemeSyntax(ELexemeType::BoxBrack, rd, currentLine, stream.get_cur() - lineStartPos));
            continue;
        }
        else if (rd == '(' || rd == ')') {
            stream.seek(1);
            result.push_back(LexemeSyntax(ELexemeType::RoundBrack, rd, currentLine, stream.get_cur() - lineStartPos));
            continue;
        }
        else if (rd == ',' || rd == ';') {
            stream.seek(1);
            result.push_back(LexemeSyntax(ELexemeType::Punctuation, rd, currentLine, stream.get_cur() - lineStartPos));
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
            result.push_back(LexemeSyntax(rd == '"' ? ELexemeType::LiteralStr : ELexemeType::LiteralChar, var_name, currentLine, stream.get_cur() - lineStartPos));
            continue;
        }
        else if (rd >= '0' && rd <= '9') {
            int cr = stream.get_cur();
            __int64 num = stream.get_num(stream.get_cur(), true, true);
            if (!stream.is_end() && stream.peek_char() == '.') {
                stream.set_cur(cr);
                double num2 = stream.get_num_dbl(stream.get_cur(), true, true);
                result.push_back(LexemeSyntax(ELexemeType::LiteralDouble, num2, currentLine, stream.get_cur() - lineStartPos));
            }
            else {
                if (abs(num) >= INT_MAX) {
                    result.push_back(LexemeSyntax(ELexemeType::LiteralNum64, num, currentLine, stream.get_cur() - lineStartPos));
                }
                else {
                    result.push_back(LexemeSyntax(ELexemeType::LiteralNum32, num, currentLine, stream.get_cur() - lineStartPos));
                }
            }
            continue;
        }
        else {
            auto checkIfInList = [&](vector<string> list, bool isOperator) -> std::tuple<bool, string> {
                int cr = stream.get_cur();
                std::tuple<bool, string> result = { false, "" };
                for (int i = 0; !stream.is_end() && list.size(); ++i) {
                    char chr = stream.read_char();
                    for (auto it = list.begin(); it != list.end(); ) {
                        if (it->at(i) != chr) {
                            it = list.erase(it);
                        }
                        else if (it->size() == i + 1) {
                            bool flag = isOperator;
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

            auto [validKeyword, str] = checkIfInList(resvKeywords, false);
            if (validKeyword) {
                stream.seek(str.size());
                result.push_back(LexemeSyntax(ELexemeType::Keyword, str, currentLine, stream.get_cur() - lineStartPos));
                continue;
            }
            auto [validOperator, str2] = checkIfInList(resvOperators, true);
            if (validOperator) {
                stream.seek(str2.size());
                result.push_back(LexemeSyntax(ELexemeType::Operator, str2, currentLine, stream.get_cur() - lineStartPos));
                continue;
            }

            string var_name;
            for (int i = 0; !stream.is_end(); ++i) {
                char c = tolower(stream.peek_char());
                if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')) break;
                var_name += stream.read_char();
            }

            if (!var_name.size()) {
                result.push_back(LexemeSyntax(ELexemeType::Invalid, currentLine, stream.get_cur() - lineStartPos));
                stream.seek(1);
            }
            else {
                result.push_back(LexemeSyntax(ELexemeType::Variable, var_name, currentLine, stream.get_cur() - lineStartPos));
            }
        }
    }

    std::ofstream out("../output.txt");
    for (auto rs : result) {
        if (rs.type == ELexemeType::Null) continue;
        out << g_LexemeTypeToStr[rs.type] << " " << LexemeTypeDataToStr(rs) << " " << rs.line << endl;
        //cout << g_LexemeTypeToStr[rs.type] << " " << LexemeTypeDataToStr(rs) << endl;
    }
    out << "End";
    out.close();
    cin.get();

    Parser parser("../output.txt");
    parser.Check();
}