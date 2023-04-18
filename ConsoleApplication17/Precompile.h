#pragma once
#include "Runtime.h"

class Precompile
{
private:
	static RuntimeType* Type_Null();

	static RuntimeType* Type_Int64();

public:
	static void CreateTypes(RuntimeCtx* ctx);
};

