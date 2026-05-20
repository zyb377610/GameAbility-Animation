#include "NePyInsightProfiler.h"

#if UE_TRACE_ENABLED

#include "Trace/Detail/LogScope.h"
#include "ProfilingDebugging/LoadTimeTracker.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "HAL/IConsoleManager.h"

#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 11
#define Py_BUILD_CORE
#include "cpython/code.h"
#include "internal/pycore_frame.h"
#include "internal/pycore_fileutils.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5
#define NEPY_INSIGHT_TRACE_NAMESPACE UE::Trace
#else
#define NEPY_INSIGHT_TRACE_NAMESPACE Trace
#endif

static TAutoConsoleVariable<int32> CVarNepyPythonInsightMode(
	TEXT("r.NepyPythonInsight.Mode"),
	1,
	TEXT("0: Every python event uses the same EventName\n")
	TEXT("1: Each python event uses its own function name as its own event name\n"),
	ECVF_ReadOnly);

namespace NepyInsight
{
	UE_TRACE_EVENT_BEGIN(Cpu, NepyPythonCall, NoSync)
		UE_TRACE_EVENT_FIELD(NEPY_INSIGHT_TRACE_NAMESPACE::AnsiString, FileName)
		UE_TRACE_EVENT_FIELD(NEPY_INSIGHT_TRACE_NAMESPACE::AnsiString, FunctionName)
		UE_TRACE_EVENT_FIELD(int32, Line)
	UE_TRACE_EVENT_END()

	TMap<FString, TArray<TUniquePtr<NEPY_INSIGHT_TRACE_NAMESPACE::Private::FScopedStampedLogScope>>> ScopeMap;
	TMap<FString, TArray<TUniquePtr<FCpuProfilerTrace::FDynamicEventScope>>> ScopeMap2;

	int PythonZoneBegin(const char* file_name, const char* func_name, int line_no)
	{
		if (bool(CpuChannel))
		{
#if ENGINE_MAJOR_VERSION >= 5
			if (auto LogScope = FCpuNepyPythonCallFields::LogScopeType::ScopedStampedEnter<FCpuNepyPythonCallFields>())
#else
			if (auto LogScope = NEPY_INSIGHT_TRACE_NAMESPACE::Private::TLogScope<FCpuNepyPythonCallFields>::ScopedStampedEnter())
#endif
			{
				TUniquePtr<NEPY_INSIGHT_TRACE_NAMESPACE::Private::FScopedStampedLogScope> NewScope = TUniquePtr<NEPY_INSIGHT_TRACE_NAMESPACE::Private::FScopedStampedLogScope>(new NEPY_INSIGHT_TRACE_NAMESPACE::Private::FScopedStampedLogScope());
				const auto& RESTRICT PythonCallInternal = *(FCpuNepyPythonCallFields*)(&LogScope);
				NewScope->SetActive();
				LogScope += LogScope
					<< PythonCallInternal.FileName(file_name)
					<< PythonCallInternal.FunctionName(func_name)
					<< PythonCallInternal.Line(line_no);

				FString Key = FString::Printf(TEXT("%s_%s"), UTF8_TO_TCHAR(file_name), UTF8_TO_TCHAR(func_name));
				auto* Array = &ScopeMap.FindOrAdd(Key);
				Array->Add(std::move(NewScope));
				return 0;
			}
			else
			{
				// EventKey要加上行号这样好排查位置 但是Key不要加行号因为Begin/End的行数是不一样的
				FString Key = FString::Printf(TEXT("%s (%s)"), UTF8_TO_TCHAR(func_name), UTF8_TO_TCHAR(file_name));
				FString EventKey = FString::Printf(TEXT("%s (%s, %d)"), UTF8_TO_TCHAR(func_name), UTF8_TO_TCHAR(file_name), line_no);
#if ENGINE_MAJOR_VERSION >= 5
				TUniquePtr<FCpuProfilerTrace::FDynamicEventScope> NewScope = TUniquePtr<FCpuProfilerTrace::FDynamicEventScope>(new FCpuProfilerTrace::FDynamicEventScope(*EventKey, CpuChannel, true));
#else
				TUniquePtr<FCpuProfilerTrace::FDynamicEventScope> NewScope = TUniquePtr<FCpuProfilerTrace::FDynamicEventScope>(new FCpuProfilerTrace::FDynamicEventScope(*EventKey, CpuChannel));
#endif
				auto* Array = &ScopeMap2.FindOrAdd(Key);
				Array->Add(std::move(NewScope));
				return 0;
			}
		}
		return -1;
	}

	int PythonZoneEnd(const char* file_name, const char* func_name, int line_no)
	{
		if (CVarNepyPythonInsightMode.GetValueOnAnyThread() == 0)
		{
			FString Key = FString::Printf(TEXT("%s_%s"), UTF8_TO_TCHAR(file_name), UTF8_TO_TCHAR(func_name));
			auto* Array = ScopeMap.Find(Key);
			if (Array && Array->Num() > 0)
			{
				Array->Pop();
				if (Array->Num() == 0)
				{
					ScopeMap.Remove(Key);
				}
			}
		}
		else
		{
			FString Key = FString::Printf(TEXT("%s (%s)"), UTF8_TO_TCHAR(func_name), UTF8_TO_TCHAR(file_name));
			auto* Array = ScopeMap2.Find(Key);
			if (Array && Array->Num() > 0)
			{
				Array->Pop();
				if (Array->Num() == 0)
				{
					ScopeMap2.Remove(Key);
				}
			}
		}
		return 0;
	}

	int Tracer(PyObject* self, PyFrameObject* f, int what, PyObject* args)
	{
		static const char* file_name = nullptr;
		static const char* func_name = nullptr;
		static int line_no = -1;

#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION >= 11
		file_name = PyUnicode_AsUTF8(f->f_frame->f_code->co_filename);
		func_name = PyUnicode_AsUTF8(f->f_frame->f_code->co_name);
		line_no = f->f_frame->f_code->co_firstlineno;
#else
		file_name = PyUnicode_AsUTF8(f->f_code->co_filename);
		func_name = PyUnicode_AsUTF8(f->f_code->co_name);
		line_no = f->f_code->co_firstlineno;
#endif
#else
		file_name = PyString_AsString(f->f_code->co_filename);
		func_name = PyString_AsString(f->f_code->co_name);
		line_no = f->f_code->co_firstlineno;
#endif

		switch (what)
		{
			case PyTrace_CALL:
			{
				PythonZoneBegin(file_name, func_name, line_no);
			}
			break;
			case PyTrace_RETURN:
			{
				PythonZoneEnd(file_name, func_name, line_no);
			}
			break;
			/* ignore PyTrace_EXCEPTION */
			default: break;
		}
		return 0;
	}
}

#endif