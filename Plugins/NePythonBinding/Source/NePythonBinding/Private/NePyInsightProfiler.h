#pragma once

#include "NePyIncludePython.h"
#include "Runtime/Launch/Resources/Version.h"
#include "frameobject.h"
#include "Trace/Config.h"

#if UE_TRACE_ENABLED

namespace NepyInsight
{
	struct ScopePythonGIL
	{
		PyGILState_STATE state;
		ScopePythonGIL()
		{
			state = PyGILState_Ensure();
		}
		~ScopePythonGIL()
		{
			PyGILState_Release(state);
		}
	};

	extern int Tracer(PyObject* self, PyFrameObject* f, int what, PyObject* args);
}

#define NEPY_INSIGHT_ENABLE_PYTHON_PROFILE { NepyInsight::ScopePythonGIL gil; PyEval_SetProfile(NepyInsight::Tracer, nullptr); }
#define NEPY_INSIGHT_DISABLE_PYTHON_PROFILE { NepyInsight::ScopePythonGIL gil; PyEval_SetProfile(nullptr, nullptr); }

#else

#define NEPY_INSIGHT_ENABLE_PYTHON_PROFILE
#define NEPY_INSIGHT_DISABLE_PYTHON_PROFILE

#endif