#pragma once
#include "Runtime.h"

class Precompile
{
private:
	static RuntimeType* Type_Null();
	static RuntimeType* Type_Int64();
	static RuntimeType* Type_String();


	static void AddReservedMethods(RuntimeCtx* ctx);

public:
	static void CreateTypes(RuntimeCtx* ctx);
};

