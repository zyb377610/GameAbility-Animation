// Copyright Epic Games, Inc. All Rights Reserved.
// 这个文件是ue_4.25\Engine\Plugins\Experimental\PythonScriptPlugin\Source\PythonScriptPlugin\Private\PyGIL.h拷贝过来的
#pragma once

#include "NePyIncludePython.h"

/** Utility to handle taking a releasing the Python GIL within a scope */
class NEPYTHONBINDING_API FNePyScopedGIL
{
public:
	/** Constructor - take the GIL */
	FNePyScopedGIL()
	{
		bIsHoldGIL = false;
		if(Py_IsInitialized())
		{
			GILState = PyGILState_Ensure();
			bIsHoldGIL = true;
		}
	}

	/** Destructor - release the GIL */
	~FNePyScopedGIL()
	{
		if(bIsHoldGIL && Py_IsInitialized())
		{
			PyGILState_Release(GILState);
		}
	}

	/** Non-copyable */
	FNePyScopedGIL(const FNePyScopedGIL&) = delete;
	FNePyScopedGIL& operator=(const FNePyScopedGIL&) = delete;

	FORCEINLINE bool IsHoldGIL() const
	{
		return bIsHoldGIL;
	}

private:
	/** Internal GIL state */
	PyGILState_STATE GILState;
	bool bIsHoldGIL;
};

