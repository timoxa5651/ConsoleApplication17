#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <array>
#include <unordered_map>
#include <stack>
#include "Poliz.h"

class Poliz;

using TID = uint64_t;
using REG = uint64_t;
using HashType = uint64_t;
#define INVALID_REG_VALUE ((uint64_t)-1)

enum class ERuntimeCallType {
	Ctor,

	MAX
};

class RuntimeMethod;
class RuntimeVar;
enum class ERuntimeType {
	Null,
	//Int32, ))
	Int64,
	Double,
	Char,
	String,
	Array,

	Custom
};

struct RuntimeParamPack {
	std::vector<RuntimeVar*> vars;
};
class RuntimeType {
private:
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

	RuntimeType(std::string name, ERuntimeType type, uint32_t size) : size(size), id(std::hash<std::string>{}(name)), type(type) {
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
	
	RuntimeMethod() : va(INVALID_REG_VALUE) {}
	static RuntimeMethod* FromPoliz(RuntimeCtx* ctx, const std::vector<PolizEntry>& poliz);

private:
	REG va;
};

struct RuntimeInstr {
	PolizCmd type;
	std::string param;
};

class RuntimeCtx;
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

	void AddPoliz(Poliz* root);
	void AddType(RuntimeType* type);
	

	RuntimeInstr* GetInstr(REG idx);
	REG AllocateFunction(size_t size);
private:
	std::map<HashType, RuntimeMethod*> regMethods;
	std::map<TID, RuntimeType*> regTypes;
	std::vector<RuntimeInstr> instrHolder;

	RuntimeExecutor* executor;
};

