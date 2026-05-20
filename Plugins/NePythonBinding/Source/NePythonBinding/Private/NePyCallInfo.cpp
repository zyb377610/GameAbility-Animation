// NePyCallInfo.cpp

#include "NePyCallInfo.h"
#include "NePyBase.h"
#include "NePyGIL.h"
#include "Misc/Paths.h"

// Thread-local storage for optimization
thread_local FNePyCallInfo* FNePyCallInfo::CachedStack = nullptr;
thread_local uint32 FNePyCallInfo::LastFrameCount = 0;
thread_local PyFrameObject* FNePyCallInfo::LastTopFrame = nullptr;

// FNePyFrameInfo implementation

FNePyFrameInfo::FNePyFrameInfo(const FNePyFrameInfo& Other)
	: PyFunctionName(Other.PyFunctionName)
	, PyFileName(Other.PyFileName)
	, FunctionName(Other.FunctionName)
	, FileName(Other.FileName)
	, LineNumber(Other.LineNumber)
	, FrameFlags(Other.FrameFlags)
	, CachedHash(Other.CachedHash)
	, LocalVariables(Other.LocalVariables)
	, ClosureVariables(Other.ClosureVariables)
{
	if (PyFunctionName)
	{
		Py_INCREF(PyFunctionName);
	}
	if (PyFileName)
	{
		Py_INCREF(PyFileName);
	}
}

FNePyFrameInfo& FNePyFrameInfo::operator=(const FNePyFrameInfo& Other)
{
	if (this != &Other)
	{
		// Decrement old references
		if (PyFunctionName)
		{
			Py_DECREF(PyFunctionName);
		}
		if (PyFileName)
		{
			Py_DECREF(PyFileName);
		}
		
		// Copy fields
		PyFunctionName = Other.PyFunctionName;
		PyFileName = Other.PyFileName;
		FunctionName = Other.FunctionName;
		FileName = Other.FileName;
		LineNumber = Other.LineNumber;
		FrameFlags = Other.FrameFlags;
		CachedHash = Other.CachedHash;
		LocalVariables = Other.LocalVariables;
		ClosureVariables = Other.ClosureVariables;
		
		// Increment new references
		if (PyFunctionName)
		{
			Py_INCREF(PyFunctionName);
		}
		if (PyFileName)
		{
			Py_INCREF(PyFileName);
		}
	}
	return *this;
}

FNePyFrameInfo::FNePyFrameInfo(FNePyFrameInfo&& Other) noexcept
	: PyFunctionName(Other.PyFunctionName)
	, PyFileName(Other.PyFileName)
	, FunctionName(MoveTemp(Other.FunctionName))
	, FileName(MoveTemp(Other.FileName))
	, LineNumber(Other.LineNumber)
	, FrameFlags(Other.FrameFlags)
	, CachedHash(Other.CachedHash)
	, LocalVariables(MoveTemp(Other.LocalVariables))
	, ClosureVariables(MoveTemp(Other.ClosureVariables))
{
	Other.PyFunctionName = nullptr;
	Other.PyFileName = nullptr;
	Other.CachedHash = 0;
}

FNePyFrameInfo& FNePyFrameInfo::operator=(FNePyFrameInfo&& Other) noexcept
{
	if (this != &Other)
	{
		// Decrement old references
		if (PyFunctionName)
		{
			Py_DECREF(PyFunctionName);
		}
		if (PyFileName)
		{
			Py_DECREF(PyFileName);
		}
		
		// Move fields
		PyFunctionName = Other.PyFunctionName;
		PyFileName = Other.PyFileName;
		FunctionName = MoveTemp(Other.FunctionName);
		FileName = MoveTemp(Other.FileName);
		LineNumber = Other.LineNumber;
		FrameFlags = Other.FrameFlags;
		CachedHash = Other.CachedHash;
		LocalVariables = MoveTemp(Other.LocalVariables);
		ClosureVariables = MoveTemp(Other.ClosureVariables);
		
		// Clear other
		Other.PyFunctionName = nullptr;
		Other.PyFileName = nullptr;
		Other.CachedHash = 0;
	}
	return *this;
}

FNePyFrameInfo::~FNePyFrameInfo()
{
	if (!Py_IsInitialized())
	{
		return;
	}
	if (PyFunctionName)
	{
		Py_DECREF(PyFunctionName);
		PyFunctionName = nullptr;
	}
	if (PyFileName)
	{
		Py_DECREF(PyFileName);
		PyFileName = nullptr;
	}
}

bool FNePyFrameInfo::InitializeFromPyFrame(PyFrameObject* Frame)
{
	if (!Frame)
	{
		return false;
	}
	
	PyCodeObject* Code = nullptr;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
	Code = PyFrame_GetCode(Frame);
#else
	Code = Frame->f_code;
	if (Code)
	{
		Py_INCREF(Code);
	}
#endif
	
	if (!Code)
	{
		return false;
	}
	
	// Store Python object references
	PyFunctionName = Code->co_name;
	if (PyFunctionName)
	{
		Py_INCREF(PyFunctionName);
	}
	
	PyFileName = Code->co_filename;
	if (PyFileName)
	{
		Py_INCREF(PyFileName);
	}
	
	// Get line number
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
	LineNumber = PyFrame_GetLineNumber(Frame);
#else
	LineNumber = Frame->f_lineno;
#endif
	
	// Set flags
	if (Code->co_flags & CO_GENERATOR)
	{
		FrameFlags |= FLAG_IS_GENERATOR;
	}
	
	Py_DECREF(Code);
	
	return true;
}

const FString& FNePyFrameInfo::GetFunctionName() const
{
	if (FunctionName.IsEmpty() && PyFunctionName)
	{
#if PY_MAJOR_VERSION >= 3
		const char* FuncName = PyUnicode_AsUTF8(PyFunctionName);
		if (FuncName)
		{
			FunctionName = UTF8_TO_TCHAR(FuncName);
		}
#else
		const char* FuncName = PyString_AsString(PyFunctionName);
		if (FuncName)
		{
			FunctionName = UTF8_TO_TCHAR(FuncName);
		}
#endif
		
		if (FunctionName.IsEmpty())
		{
			FunctionName = TEXT("<unknown>");
		}
		
		// Check if module
		if (FunctionName == TEXT("<module>"))
		{
			const_cast<FNePyFrameInfo*>(this)->FrameFlags |= FLAG_IS_MODULE;
		}
	}
	return FunctionName;
}

const FString& FNePyFrameInfo::GetFileName() const
{
	if (FileName.IsEmpty() && PyFileName)
	{
#if PY_MAJOR_VERSION >= 3
		const char* FileNameStr = PyUnicode_AsUTF8(PyFileName);
		if (FileNameStr)
		{
			FString FullPath = UTF8_TO_TCHAR(FileNameStr);
			FileName = FPaths::GetCleanFilename(FullPath);
		}
#else
		const char* FileNameStr = PyString_AsString(PyFileName);
		if (FileNameStr)
		{
			FString FullPath = UTF8_TO_TCHAR(FileNameStr);
			FileName = FPaths::GetCleanFilename(FullPath);
		}
#endif
		
		if (FileName.IsEmpty())
		{
			FileName = TEXT("<unknown>");
		}
		
		// Check if builtin
		if (FileName.Contains(TEXT("<built-in>")))
		{
			const_cast<FNePyFrameInfo*>(this)->FrameFlags |= FLAG_IS_BUILTIN;
		}
	}
	return FileName;
}

FString FNePyFrameInfo::ToString(int32 Depth) const
{
	FString Indent;
	for (int32 i = 0; i < Depth; ++i)
	{
		Indent += TEXT("  ");
	}

	FString Result;
	Result += FString::Printf(TEXT("%sFrame #%d: %s (%s:%d)\n"), 
		*Indent, Depth, *GetFunctionName(), *GetFileName(), LineNumber);

	// Add flags
	TArray<FString> FlagNames;
	if (IsBuiltin()) FlagNames.Add(TEXT("Builtin"));
	if (IsModule()) FlagNames.Add(TEXT("Module"));
	if (IsGenerator()) FlagNames.Add(TEXT("Generator"));
	if (FlagNames.Num() > 0)
	{
		Result += FString::Printf(TEXT("%s  Flags: %s\n"), *Indent, *FString::Join(FlagNames, TEXT(", ")));
	}

	// Add local variables
	if (LocalVariables.Num() > 0)
	{
		Result += FString::Printf(TEXT("%s  Local Variables (%d):\n"), *Indent, LocalVariables.Num());
		for (const FNePyVariableInfo& Var : LocalVariables)
		{
			Result += FString::Printf(TEXT("%s    %s\n"), *Indent, *Var.ToString());
		}
	}

	// Add closure variables
	if (ClosureVariables.Num() > 0)
	{
		Result += FString::Printf(TEXT("%s  Closure Variables (%d):\n"), *Indent, ClosureVariables.Num());
		for (const FNePyVariableInfo& Var : ClosureVariables)
		{
			Result += FString::Printf(TEXT("%s    %s\n"), *Indent, *Var.ToString());
		}
	}

	return Result;
}

// FNePyCallInfo implementation

FNePyCallInfo::FNePyCallInfo()
	: ThreadId(0)
	, CachedHash(0)
{
}

FNePyCallInfo::~FNePyCallInfo()
{
}

void FNePyCallInfo::CleanupThreadLocalCache()
{
	if (CachedStack)
	{
		delete CachedStack;
		CachedStack = nullptr;
	}
	LastFrameCount = 0;
	LastTopFrame = nullptr;
}

bool FNePyCallInfo::CaptureCurrentCallStack(int32 MaxDepth, bool bCaptureVariables)
{
	FNePyScopedGIL GIL;

	// Fast path: check if we can reuse cached stack
	PyThreadState* ThreadState = PyThreadState_Get();
	if (!ThreadState)
	{
		UE_LOG(LogNePython, Warning, TEXT("No current Python thread state available."));
		return false;
	}

	if (PyErr_Occurred())
	{
		UE_LOG(LogNePython, Warning, TEXT("Error in Python interpreter before capturing stack."));
		return false;
	}

	// Get current top frame based on Python version
	PyFrameObject* CurrentTopFrame = nullptr;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
	CurrentTopFrame = PyThreadState_GetFrame(ThreadState);
#else
	CurrentTopFrame = ThreadState->frame;
	if (CurrentTopFrame)
	{
		Py_INCREF(CurrentTopFrame);
	}
#endif

	if (!CurrentTopFrame)
	{
		// UE_LOG(LogNePython, Warning, TEXT("No current Python frame available."));
		return false;
	}

	// Check if stack has changed since last capture (only for non-variable capture)
	if (!bCaptureVariables && CachedStack && LastTopFrame == CurrentTopFrame &&
		CachedStack->Frames.Num() <= MaxDepth)
	{
		// Reuse cached stack data
		*this = *CachedStack;
		Py_DECREF(CurrentTopFrame);
		return true;
	}

	// Reset state for new capture
	Frames.Reset();
	CachedHash = 0;
	ThreadId = (uint8)(FPlatformTLS::GetCurrentThreadId() & 0xFF);

	if (bCaptureVariables)
	{
		// Detailed capture with variable information
		// Pre-allocate space
		Frames.Reserve(FMath::Min(MaxDepth, 16));

		PyFrameObject* CurrentFrame = CurrentTopFrame;
		Py_INCREF(CurrentFrame);

		int32 FrameCount = 0;
		while (CurrentFrame && FrameCount < MaxDepth)
		{
			FNePyFrameInfo FrameInfo;
			if (ExtractFrameInfo(CurrentFrame, FrameInfo))
			{
				Frames.Emplace(MoveTemp(FrameInfo));
			}

			// Get next frame
			PyFrameObject* NextFrame = nullptr;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
			NextFrame = PyFrame_GetBack(CurrentFrame);
#else
			NextFrame = CurrentFrame->f_back;
			if (NextFrame)
			{
				Py_INCREF(NextFrame);
			}
#endif

			// Release current frame reference
			Py_DECREF(CurrentFrame);

			if (!NextFrame)
			{
				break;
			}

			CurrentFrame = NextFrame;
			FrameCount++;
		}
	}
	else
	{
		// Perform optimized stack traversal without variables
		CaptureStackFramesOptimized(CurrentTopFrame, MaxDepth);

		// Update thread-local cache for lightweight captures
		UpdateThreadLocalCache(CurrentTopFrame);
	}

	Py_DECREF(CurrentTopFrame);

	CleanupThreadLocalCache();

	return Frames.Num() > 0;
}

void FNePyCallInfo::CaptureStackFramesOptimized(PyFrameObject* StartFrame, int32 MaxFrames)
{
	Frames.Reset();
	if (!StartFrame)
	{
		return;
	}
	
	Frames.Reserve(FMath::Min(MaxFrames, 16));
	
	PyFrameObject* CurrentFrame = StartFrame;
	Py_INCREF(CurrentFrame);
	
	int32 FrameCount = 0;
	while (CurrentFrame && FrameCount < MaxFrames)
	{
		FNePyFrameInfo FrameInfo;
		if (FrameInfo.InitializeFromPyFrame(CurrentFrame))
		{
			Frames.Emplace(MoveTemp(FrameInfo));
		}
		
		// Get next frame
		PyFrameObject* NextFrame = nullptr;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
		NextFrame = PyFrame_GetBack(CurrentFrame);
#else
		NextFrame = CurrentFrame->f_back;
		if (NextFrame)
		{
			Py_INCREF(NextFrame);
		}
#endif
		
		Py_DECREF(CurrentFrame);
		
		if (!NextFrame)
		{
			break;
		}
		
		CurrentFrame = NextFrame;
		FrameCount++;
	}

	CleanupThreadLocalCache();
}

void FNePyCallInfo::UpdateThreadLocalCache(PyFrameObject* TopFrame)
{
	if (!CachedStack)
	{
		CachedStack = new FNePyCallInfo();
	}	
	// Copy current stack data to cache
	*CachedStack = *this;	
	LastTopFrame = TopFrame;	
	LastFrameCount = Frames.Num();
}

bool FNePyCallInfo::ExtractFrameInfo(PyFrameObject* Frame, FNePyFrameInfo& OutFrameInfo)
{
	if (!Frame)
	{
		return false;
	}

	PyCodeObject* Code = nullptr;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 11
	Code = PyFrame_GetCode(Frame);
#else
	Code = Frame->f_code;
	if (Code)
	{
		Py_INCREF(Code);
	}
#endif

	if (!Code)
	{
		return false;
	}

	// Store Python object references
	OutFrameInfo.PyFunctionName = Code->co_name;
	if (OutFrameInfo.PyFunctionName)
	{
		Py_INCREF(OutFrameInfo.PyFunctionName);
	}
	
	OutFrameInfo.PyFileName = Code->co_filename;
	if (OutFrameInfo.PyFileName)
	{
		Py_INCREF(OutFrameInfo.PyFileName);
	}
	
	// Get line number
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
	OutFrameInfo.LineNumber = PyFrame_GetLineNumber(Frame);
#else
	OutFrameInfo.LineNumber = Frame->f_lineno;
#endif
	
	// Set flags
	if (Code->co_flags & CO_GENERATOR)
	{
		OutFrameInfo.FrameFlags |= FNePyFrameInfo::FLAG_IS_GENERATOR;
	}

	if (OutFrameInfo.GetFunctionName() == TEXT("<module>"))
	{
		OutFrameInfo.FrameFlags |= FNePyFrameInfo::FLAG_IS_MODULE;
	}

	if (OutFrameInfo.GetFileName().Contains(TEXT("<built-in>")))
	{
		OutFrameInfo.FrameFlags |= FNePyFrameInfo::FLAG_IS_BUILTIN;
	}

	// Extract variables
	ExtractLocalVariables(Frame, OutFrameInfo);
	ExtractClosureVariables(Frame, Code, OutFrameInfo);

	if (OutFrameInfo.LocalVariables.Num() > 0 || OutFrameInfo.ClosureVariables.Num() > 0)
	{
		OutFrameInfo.FrameFlags |= FNePyFrameInfo::FLAG_HAS_LOCALS;
	}

	Py_DECREF(Code);

	return true;
}

void FNePyCallInfo::ExtractLocalVariables(PyFrameObject* Frame, FNePyFrameInfo& OutFrameInfo)
{
	if (!Frame)
	{
		return;
	}

	// Get locals dictionary
	PyObject* Locals = nullptr;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 11
	Locals = PyFrame_GetLocals(Frame);
#else
	Locals = Frame->f_locals;
	if (Locals)
	{
		Py_INCREF(Locals);
	}
#endif

	if (!Locals || !PyDict_Check(Locals))
	{
		if (Locals)
		{
			Py_DECREF(Locals);
		}
		return;
	}

	PyObject* Key;
	PyObject* Value;
	Py_ssize_t Pos = 0;

	while (PyDict_Next(Locals, &Pos, &Key, &Value))
	{
		FNePyVariableInfo VarInfo;
		VarInfo.Flags |= FNePyVariableInfo::FLAG_IS_LOCAL;

		// Get variable name
#if PY_MAJOR_VERSION >= 3
		if (PyUnicode_Check(Key))
		{
			const char* KeyStr = PyUnicode_AsUTF8(Key);
			if (KeyStr)
			{
				VarInfo.Name = UTF8_TO_TCHAR(KeyStr);
			}
		}
#else
		if (PyString_Check(Key))
		{
			const char* KeyStr = PyString_AsString(Key);
			if (KeyStr)
			{
				VarInfo.Name = UTF8_TO_TCHAR(KeyStr);
			}
		}
#endif

		// Get type name
		VarInfo.TypeName = GetPyObjectTypeName(Value);

		// Get value
		VarInfo.Value = PyObjectToString(Value);

		// Get reference count - store as-is (including UINT_MAX for immortal objects)
		VarInfo.RefCount = Value ? Py_REFCNT(Value) : 0;

		// Check for None
		if (Value == Py_None)
		{
			VarInfo.Flags |= FNePyVariableInfo::FLAG_IS_NONE;
		}

		OutFrameInfo.LocalVariables.Add(MoveTemp(VarInfo));
	}

	Py_DECREF(Locals);
}

void FNePyCallInfo::ExtractClosureVariables(PyFrameObject* Frame, PyCodeObject* Code, FNePyFrameInfo& OutFrameInfo)
{
	if (!Frame || !Code)
	{
		return;
	}

	// Get free variables tuple (variables from outer scopes)
	PyObject* FreeVars = nullptr;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 11
	// Python 3.11+ changed internal structure
	FreeVars = PyCode_GetFreevars(Code);
#else
	FreeVars = Code->co_freevars;
	if (FreeVars)
	{
		Py_INCREF(FreeVars);
	}
#endif

	if (!FreeVars || !PyTuple_Check(FreeVars))
	{
		if (FreeVars)
		{
			Py_DECREF(FreeVars);
		}
		return;
	}

	Py_ssize_t FreeVarCount = PyTuple_Size(FreeVars);
	if (FreeVarCount == 0)
	{
		Py_DECREF(FreeVars);
		return;
	}

#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 11
	// For Python 3.11+, use frame locals to get closure values
	PyObject* Locals = PyFrame_GetLocals(Frame);
#else
	PyObject* Locals = Frame->f_locals;
	if (Locals)
	{
		Py_INCREF(Locals);
	}
#endif
	if (Locals && PyDict_Check(Locals))
	{
		for (Py_ssize_t i = 0; i < FreeVarCount; ++i)
		{
			PyObject* VarName = PyTuple_GetItem(FreeVars, i);
			if (!VarName || !PyUnicode_Check(VarName))
			{
				continue;
			}

			FNePyVariableInfo VarInfo;
			VarInfo.Flags |= FNePyVariableInfo::FLAG_IS_CLOSURE;

#if PY_MAJOR_VERSION >= 3
			if (PyUnicode_Check(VarName))
			{
				const char* VarNameStr = PyUnicode_AsUTF8(VarName);
				if (VarNameStr)
				{
					VarInfo.Name = UTF8_TO_TCHAR(VarNameStr);
				}
			}
#else
			if (PyString_Check(VarName))
			{
				const char* VarNameStr = PyString_AsString(VarName);
				if (VarNameStr)
				{
					VarInfo.Name = UTF8_TO_TCHAR(VarNameStr);
				}
			}
#endif

			// Try to get value from locals
			PyObject* Value = PyDict_GetItem(Locals, VarName);
			if (Value)
			{
				VarInfo.TypeName = GetPyObjectTypeName(Value);
				VarInfo.Value = PyObjectToString(Value);
				// Store refcount as-is (including UINT_MAX for immortal objects)
				VarInfo.RefCount = Py_REFCNT(Value);

				if (Value == Py_None)
				{
					VarInfo.Flags |= FNePyVariableInfo::FLAG_IS_NONE;
				}
			}
			else
			{
				VarInfo.TypeName = TEXT("upvalue");
				VarInfo.Value = TEXT("<not in locals>");
				VarInfo.RefCount = 0;
			}

			OutFrameInfo.ClosureVariables.Add(MoveTemp(VarInfo));
		}
	}

	if (Locals)
	{
		Py_DECREF(Locals);
	}

	Py_DECREF(FreeVars);
}

FString FNePyCallInfo::PyObjectToString(PyObject* Obj, int32 MaxLength)
{
	if (!Obj)
	{
		return TEXT("<null>");
	}

	if (Obj == Py_None)
	{
		return TEXT("None");
	}

	// Try to convert to string using repr
	PyObject* ReprObj = PyObject_Repr(Obj);
	if (!ReprObj)
	{
		PyErr_Clear();
		return TEXT("<repr failed>");
	}

	FString Result;
#if PY_MAJOR_VERSION >= 3
	const char* ReprStr = PyUnicode_AsUTF8(ReprObj);
	if (ReprStr)
	{
		Result = UTF8_TO_TCHAR(ReprStr);
	}
#else
	const char* ReprStr = PyString_AsString(ReprObj);
	if (ReprStr)
	{
		Result = UTF8_TO_TCHAR(ReprStr);
	}
#endif

	Py_DECREF(ReprObj);

	if (Result.IsEmpty())
	{
		return TEXT("<empty>");
	}

	// Truncate if too long
	if (MaxLength > 0 && Result.Len() > MaxLength)
	{
		Result = Result.Left(MaxLength - 3) + TEXT("...");
	}

	return Result;
}

FString FNePyCallInfo::GetPyObjectTypeName(PyObject* Obj)
{
	if (!Obj)
	{
		return TEXT("null");
	}

	if (Obj->ob_type && Obj->ob_type->tp_name)
	{
		return UTF8_TO_TCHAR(Obj->ob_type->tp_name);
	}

	return TEXT("unknown");
}

FString FNePyCallInfo::ToString(bool bIncludeVariables, bool bIncludeClosures) const
{
	if (Frames.Num() == 0)
	{
		return TEXT("(empty call stack)");
	}

	FString Result;
	Result += TEXT("=== Python Call Stack ===\n");
	Result += FString::Printf(TEXT("Total Frames: %d\n\n"), Frames.Num());

	// Display frames from innermost (bottom) to outermost (top)
	for (int32 i = 0; i < Frames.Num(); ++i)
	{
		const FNePyFrameInfo& Frame = Frames[i];
		int32 Depth = i;

		if (bIncludeVariables)
		{
			Result += Frame.ToString(Depth);
		}
		else
		{
			FString Indent;
			for (int32 j = 0; j < Depth; ++j)
			{
				Indent += TEXT("  ");
			}
			Result += FString::Printf(TEXT("%sFrame #%d: %s (%s:%d)\n"), 
				*Indent, Depth, *Frame.FunctionName, *Frame.FileName, Frame.LineNumber);
		}

		if (i < Frames.Num() - 1)
		{
			Result += TEXT("\n");
		}
	}

	return Result;
}

bool FNePyCallInfo::CaptureFromTraceback(PyObject* PyTraceback, int32 MaxDepth, bool bCaptureVariables)
{
	FNePyScopedGIL GIL;

	if (!PyTraceback || !PyTraceBack_Check(PyTraceback))
	{
		UE_LOG(LogNePython, Warning, TEXT("Invalid traceback object."));
		return false;
	}

	// Reset state for new capture
	Frames.Reset();
	CachedHash = 0;
	ThreadId = (uint8)(FPlatformTLS::GetCurrentThreadId() & 0xFF);

	// Pre-allocate space
	Frames.Reserve(FMath::Min(MaxDepth, 16));

	// Iterate through traceback chain
	PyTracebackObject* CurrentTb = (PyTracebackObject*)PyTraceback;
	int32 FrameCount = 0;

	while (CurrentTb && FrameCount < MaxDepth)
	{
		// Get frame from traceback
		PyFrameObject* Frame = CurrentTb->tb_frame;
		if (!Frame)
		{
			break;
		}

		FNePyFrameInfo FrameInfo;
		if (bCaptureVariables)
		{
			// Extract detailed frame info with variables
			if (ExtractFrameInfo(Frame, FrameInfo))
			{
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
				FrameInfo.LineNumber = PyFrame_GetLineNumber(Frame);
#else
				FrameInfo.LineNumber = Frame->f_lineno;
#endif
				Frames.Add(FrameInfo);
			}
		}
		else
		{
			// Lightweight capture without variables
			if (FrameInfo.InitializeFromPyFrame(Frame))
			{
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9
				FrameInfo.LineNumber = PyFrame_GetLineNumber(Frame);
#else
				FrameInfo.LineNumber = Frame->f_lineno;
#endif
				Frames.Add(FrameInfo);
			}
		}

		// Move to next traceback
		CurrentTb = CurrentTb->tb_next;
		FrameCount++;
	}

	// Reverse the frames array to match the display order in ToString
	// This makes both CaptureCurrentCallStack and CaptureFromTraceback consistent
	Algo::Reverse(Frames);

	return Frames.Num() > 0;
}

namespace NePyCallInfo
{
	FString GetCurrentCallInfo(bool bIncludeVariables, bool bIncludeClosure, int32 MaxDepth)
	{
		FNePyCallInfo CallInfo;
		if (!CallInfo.CaptureCurrentCallStack(MaxDepth))
		{
			PyErr_Format(PyExc_RuntimeError, "Failed to capture current call stack.");
			return TEXT("(failed to capture call stack)");
		}
		return CallInfo.ToString(bIncludeVariables, bIncludeClosure);
	}

	FString GetTracebackCallInfo(PyObject* PyTraceback, bool bIncludeVariables, bool bIncludeClosures, int32 MaxDepth)
	{
		FNePyCallInfo CallInfo;
		if (!CallInfo.CaptureFromTraceback(PyTraceback, MaxDepth, bIncludeVariables))
		{
			PyErr_Format(PyExc_RuntimeError, "Failed to capture call stack from traceback.");
			return TEXT("(failed to capture call stack from traceback)");
		}
		return CallInfo.ToString(bIncludeVariables, bIncludeClosures);
	}

	bool IsGILHeldByOtherThread()
	{
		if (!Py_IsInitialized())
		{
			return false;
		}

		if (PyGILState_Check() != 0)
		{
			// 当前线程持有GIL，返回false
			return false;
		}

		// 当前线程未持有GIL，检查是否有其他线程持有
		PyInterpreterState* Interp = PyInterpreterState_Head();
		if (Interp == nullptr)
		{
			// 没有解释器状态，没有线程持有GIL
			return false;
		}

		// 遍历所有Python线程状态
		PyThreadState* ThreadState = PyInterpreterState_ThreadHead(Interp);
		while (ThreadState != nullptr)
		{
			// 检查线程的GIL计数器
			// gilstate_counter > 0 表示该线程持有GIL
			if (ThreadState->gilstate_counter > 0)
			{
				// 找到持有GIL的线程
				UE_LOG(LogNePython, Verbose,
					TEXT("[IsGILHoldByOtherThread] GIL is held by thread %lu (counter: %d)"),
					(unsigned long)ThreadState->thread_id,
					(int32)ThreadState->gilstate_counter);

				return true; // GIL被其他线程持有
			}

			ThreadState = PyThreadState_Next(ThreadState);
		}

		// 没有线程持有GIL
		return false;
	}

	void PrintCurrentCallInfo()
	{
		if (!Py_IsInitialized())
		{
			UE_LOG(LogNePython, Warning, TEXT("PrintCurrentCallInfo: Python interpreter is not initialized, cannot print call info."));
			return;
		}
		FString CallInfoStr = GetCurrentCallInfo(true, true, 65536);
		UE_LOG(LogNePython, Display, TEXT("%s"), *CallInfoStr);
	}

	void SafePrintCurrentCallInfo()
	{
		if (!Py_IsInitialized())
		{
			UE_LOG(LogNePython, Warning, TEXT("SafePrintCurrentCallInfo: Python interpreter is not initialized, cannot print call info."));
			return;
		}
		if (IsGILHeldByOtherThread())
		{
			UE_LOG(LogNePython, Warning, TEXT("SafePrintCurrentCallInfo: GIL is held by other thread, cannot print call info."));
			return;
		}
		FString CallInfoStr = GetCurrentCallInfo(true, true, 65536);
		UE_LOG(LogNePython, Display, TEXT("%s"), *CallInfoStr);
	}
}