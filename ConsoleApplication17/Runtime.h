#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <array>
#include <unordered_map>
#include <stack>
#include <any>
#include <cassert>

#include "Poliz.h"

class Poliz;
class Parser;

using TID = uint64_t;
using REG = uint64_t;
using HashType = uint64_t;
using Hash = std::hash<std::string>;
#define INVALID_REG_VALUE ((uint64_t)-1)

enum class ERuntimeCallType {
	Invalid,

	Assign,
	Add,
	Sub,
	Mult,
	Div,

	CompareEq,

	MAX
};
inline std::string ERuntimeCallType_ToString(ERuntimeCallType c) {
	switch (c) {
	case ERuntimeCallType::Assign: return "Assign";
	case ERuntimeCallType::Add: return "Add";
	case ERuntimeCallType::Sub: return "Sub";
	case ERuntimeCallType::Mult: return "Mult";
	case ERuntimeCallType::Div: return "Div";
	case ERuntimeCallType::CompareEq: return "CompareEq";
	}
	return "";
}
inline ERuntimeCallType ERuntimeCallType_FromString(std::string str) {
	if (str == "=") {
		return ERuntimeCallType::Assign;
	}
	else if (str == "+") {
		return ERuntimeCallType::Add;
	}
	else if (str == "-") {
		return ERuntimeCallType::Sub;
	}
	else if (str == "/") {
		return ERuntimeCallType::Div;
	}
	else if (str == "*") {
		return ERuntimeCallType::Mult;
	}
	else if (str == "==") {
		return ERuntimeCallType::CompareEq;
	}
	assert(false);
	return ERuntimeCallType::Invalid;
}

class RuntimeMethod;
class RuntimeVar;
class RuntimeCtx;

enum class ERuntimeType {
	Null,
	//Int32, ))
	Int64,
	Double,
	String,
	Array,

	Custom
};

struct RuntimeParamPack {
	std::vector<RuntimeVar*> vars;
};
class RuntimeType {
private:
	std::string name;
	TID id;
	ERuntimeType type;
	uint32_t size;

	std::function<void(RuntimeVar*, void*)> nativeCtor;
	std::array<void*, static_cast<int>(ERuntimeCallType::MAX)> vtable;

public:
	void SetNativeCtor(decltype(RuntimeType::nativeCtor) func) {
		this->nativeCtor = func;
	}

	template<typename T>
	void SetMethod(ERuntimeCallType type, T func) {
		this->vtable[static_cast<int>(type)] = func;
	}

	bool IsNativeCtorable() {
		return this->nativeCtor != nullptr;
	}
	TID GetTID() {
		return this->id;
	}
	std::string GetName() {
		return this->name;
	}

	RuntimeType(std::string name, ERuntimeType type, uint32_t size) : size(size), name(name), id(std::hash<std::string>{}(name)), type(type) {
		this->nativeCtor = nullptr;
		vtable.fill(0);
	}
};

class RuntimeVar {
public:
	RuntimeType* heldType;
	uint8_t* dataBlob;
};

class RuntimeMethod {
public:
	
	RuntimeMethod(std::string name, int paramCnt) : va(INVALID_REG_VALUE), name(name), params(paramCnt) {}
	void FromPoliz(RuntimeCtx* ctx, const std::vector<PolizEntry>& poliz);

	int GetParamCount() {
		return this->params;
	}
	std::string GetName() {
		return this->name;
	}
private:
	std::string name;
	int params;

	REG va;
};

enum class RuntimeInstrType {
	Invalid = 0,

	Ctor = 1, // Ctor [ret] [tid] params...
	Operation = 2, // Operation [ret] [operation(ERuntimeCallType)] param1 param2
	Call = 3, // Call [ret] [func] params...
	Array = 4, // Array [ret] params...
	Jz = 5, // Jz [delta] [var]        // jumps If var == false!
	Jmp = 6, // Jmp [delta]
	Ret = 7, // Ret [value]
};
inline std::string RuntimeInstrType_ToString(RuntimeInstrType c) {
	switch (c) {
	case RuntimeInstrType::Invalid: return "Invalid";
	case RuntimeInstrType::Ctor: return "Ctor";
	case RuntimeInstrType::Operation: return "Operation";
	case RuntimeInstrType::Call: return "Call";
	case RuntimeInstrType::Array: return "Array";
	case RuntimeInstrType::Jz: return "Jz";
	case RuntimeInstrType::Jmp: return "Jmp";
	case RuntimeInstrType::Ret: return "Ret";
	}
	return "";
}

struct RuntimeInstr {
	RuntimeInstrType opcode;

	template<typename T>
	void AddParam(T param) {
		static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, int64_t> || std::is_same_v<T, HashType> || std::is_same_v<T, TID> || std::is_same_v<T, ERuntimeCallType>);
		this->params.push_back(param);
	}

	template<typename T>
	const T& GetParam(size_t idx) const {
		return std::any_cast<const T&>(this->params[idx]);
	}
	template<typename T>
	T& RefParam(size_t idx) {
		return std::any_cast<T&>(this->params[idx]);
	}

	size_t GetParamCount() const {
		return this->params.size();
	}
	std::string GetReturnName() const {
		return this->GetParam<std::string>(0);
	}

	std::string GetParamString(RuntimeCtx* ctx, size_t idx) const;

	RuntimeInstr(RuntimeInstrType op) : opcode(op) {}
private:
	std::vector<std::any> params;
};

class RuntimeExecutor {
private:
	REG ip;
	std::stack<REG> stack;

	RuntimeVar* ExecuteInstr(RuntimeCtx* ctx);
public:
	RuntimeVar* CallMethod(RuntimeCtx* ctx, RuntimeMethod* method, const RuntimeParamPack& params);
};

class RuntimeCtx
{
public:
	RuntimeCtx();
	~RuntimeCtx();

	void AddPoliz(Parser* parser, Poliz* root);
	void AddType(RuntimeType* type);
	
	RuntimeMethod* GetMethod(HashType methodName);
	RuntimeType* GetType(TID methodName);

	RuntimeInstr* GetInstr(REG idx);
	REG AllocateFunction(size_t size);
private:
	std::map<HashType, RuntimeMethod*> regMethods;
	std::map<TID, RuntimeType*> regTypes;
	std::vector<RuntimeInstr> instrHolder;

	RuntimeExecutor* executor;
};

