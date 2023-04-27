#pragma once
#include "Runtime.h"

class Precompile
{
private:
	static RuntimeType* Type_Null();
	static RuntimeType* Type_Int64();
	static RuntimeType* Type_Double();
	static RuntimeType* Type_String();
	static RuntimeType* Type_Array();


	static void AddReservedMethods(RuntimeCtx* ctx);

public:
	static void CreateTypes(RuntimeCtx* ctx);
};

