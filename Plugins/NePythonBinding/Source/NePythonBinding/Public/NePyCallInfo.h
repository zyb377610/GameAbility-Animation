// NePyCallInfo.h
#pragma once

#include "CoreMinimal.h"
#include "NePyIncludePython.h"
#include "frameobject.h"

/**
 * Represents a variable (local or closure) in a Python frame
 */
struct FNePyVariableInfo
{
	FString Name;
	FString TypeName;
	FString Value;
	int64 RefCount;  // Python object reference count
	uint8 Flags;

	enum EVariableFlags : uint8
	{
		FLAG_IS_LOCAL = 1 << 0,      // Local variable
		FLAG_IS_CLOSURE = 1 << 1,    // Closure variable
		FLAG_IS_GLOBAL = 1 << 2,     // Global variable
		FLAG_IS_BUILTIN = 1 << 3,    // Built-in variable
		FLAG_IS_NONE = 1 << 4,       // Variable is None
	};

	FNePyVariableInfo()
		: RefCount(0)
		, Flags(0)
	{
	}

	bool IsLocal() const { return (Flags & FLAG_IS_LOCAL) != 0; }
	bool IsClosure() const { return (Flags & FLAG_IS_CLOSURE) != 0; }
	bool IsGlobal() const { return (Flags & FLAG_IS_GLOBAL) != 0; }
	bool IsBuiltin() const { return (Flags & FLAG_IS_BUILTIN) != 0; }
	bool IsNone() const { return (Flags & FLAG_IS_NONE) != 0; }

	FString ToString() const
	{
		FString TypeStr;
		if (IsLocal()) TypeStr += TEXT("L");
		if (IsClosure()) TypeStr += TEXT("C");
		if (IsGlobal()) TypeStr += TEXT("G");
		if (IsBuiltin()) TypeStr += TEXT("B");
		if (TypeStr.IsEmpty()) TypeStr = TEXT("?");

		FString ValueStr = Value;
		// Truncate long values
		if (ValueStr.Len() > 100)
		{
			ValueStr = ValueStr.Left(97) + TEXT("...");
		}

		// Format reference count - handle immortal objects
		FString RefCountStr;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 12 || PY_MAJOR_VERSION >= 4
		if (RefCount == _Py_IMMORTAL_REFCNT)
		{
			RefCountStr = TEXT("(immortal)");
		}
		else
#endif
		{
			RefCountStr = FString::Printf(TEXT("(refcnt=%lld)"), RefCount);
		}

		return FString::Printf(TEXT("[%s] %s: %s = %s %s"), 
			*TypeStr, *Name, *TypeName, *ValueStr, *RefCountStr);
	}
};

/**
 * Detailed information about a single frame in the call stack
 * This structure consolidates both detailed frame info (with variables) and lightweight frame info
 */
struct FNePyFrameInfo
{
	// Python object references (for lightweight tracking - can be null)
	PyObject* PyFunctionName;
	PyObject* PyFileName;
	
	// Cached string representations
	mutable FString FunctionName;
	mutable FString FileName;
	
	int32 LineNumber;
	uint8 FrameFlags;
	mutable uint32 CachedHash;
	
	// Detailed variable information (optional - for deep stack traces)
	TArray<FNePyVariableInfo> LocalVariables;
	TArray<FNePyVariableInfo> ClosureVariables;

	enum EFrameFlags : uint8
	{
		FLAG_IS_BUILTIN = 1 << 0,
		FLAG_IS_MODULE = 1 << 1,
		FLAG_IS_GENERATOR = 1 << 2,
		FLAG_HAS_LOCALS = 1 << 3,
	};

	FNePyFrameInfo()
		: PyFunctionName(nullptr)
		, PyFileName(nullptr)
		, LineNumber(0)
		, FrameFlags(0)
		, CachedHash(0)
	{
	}
	
	// Copy constructor and assignment operator for Python object reference management
	FNePyFrameInfo(const FNePyFrameInfo& Other);
	FNePyFrameInfo& operator=(const FNePyFrameInfo& Other);
	
	// Move constructor and assignment operator
	FNePyFrameInfo(FNePyFrameInfo&& Other) noexcept;
	FNePyFrameInfo& operator=(FNePyFrameInfo&& Other) noexcept;
	
	~FNePyFrameInfo();

	// Initialize from PyFrameObject (for lightweight tracking)
	bool InitializeFromPyFrame(PyFrameObject* Frame);

	bool IsValid() const { return PyFunctionName != nullptr || !FunctionName.IsEmpty(); }
	bool IsBuiltin() const { return (FrameFlags & FLAG_IS_BUILTIN) != 0; }
	bool IsModule() const { return (FrameFlags & FLAG_IS_MODULE) != 0; }
	bool IsGenerator() const { return (FrameFlags & FLAG_IS_GENERATOR) != 0; }
	bool HasLocals() const { return (FrameFlags & FLAG_HAS_LOCALS) != 0; }
	
	// Get string representations (lazy evaluation from PyObject if needed)
	const FString& GetFunctionName() const;
	const FString& GetFileName() const;

	FString ToString(int32 Depth = 0) const;
	
	// Simplified ToString for lightweight frames (no variables)
	FString ToSimpleString() const
	{
		if (IsModule())
		{
			return FString::Printf(TEXT("%s:%d"), *GetFileName(), LineNumber);
		}
		return FString::Printf(TEXT("%s (%s:%d)"), *GetFunctionName(), *GetFileName(), LineNumber);
	}
	
	/**
	 * Format frame for folded stack representation (used in flame graphs)
	 * This format is more compact and optimized for flame graph visualization
	 * @return Folded format string: "function_name"
	 */
	FString ToFoldedString() const
	{
		// For flame graphs, we typically only need the function name
		// Modules are shown as filename without parentheses
		if (IsModule())
		{
			return GetFileName();
		}
		return GetFunctionName();
	}

	// Hash and comparison (for use in maps and deduplication)
	uint32 GetTypeHash() const
	{
		if (CachedHash == 0)
		{
			CachedHash = HashCombine(
				HashCombine((uint32)(uintptr_t)PyFunctionName, (uint32)(uintptr_t)PyFileName),
				HashCombine(::GetTypeHash(LineNumber), ::GetTypeHash(FrameFlags))
			);
		}
		return CachedHash;
	}
	
	bool operator==(const FNePyFrameInfo& Other) const
	{
		return PyFunctionName == Other.PyFunctionName &&
			PyFileName == Other.PyFileName &&
			LineNumber == Other.LineNumber &&
			FrameFlags == Other.FrameFlags;
	}
};

inline uint32 GetTypeHash(const FNePyFrameInfo& Frame)
{
	return Frame.GetTypeHash();
}

// Legacy alias for backward compatibility
using FNePyStackFrame = FNePyFrameInfo;

/**
 * Complete call stack information with detailed frame data
 * This class consolidates both detailed call stack (FNePyCallInfo) and lightweight call stack (FNePyCallStack)
 */
class FNePyCallInfo
{
public:
	FNePyCallInfo();
	~FNePyCallInfo();

	/**
	 * Capture the current Python call stack with detailed information
	 * @param MaxDepth Maximum number of frames to capture (default: 65536)
	 * @param bCaptureVariables Whether to capture variable information (default: true)
	 * @return true if successful
	 */
	bool CaptureCurrentCallStack(int32 MaxDepth = 65536, bool bCaptureVariables = true);
	
	/**
	 * Capture stack trace from a Python traceback object (for exception handling)
	 * @param PyTraceback The traceback object from an exception
	 * @param MaxDepth Maximum number of frames to capture (default: 65536)
	 * @param bCaptureVariables Whether to capture variable information (default: false)
	 * @return true if successful
	 */
	bool CaptureFromTraceback(PyObject* PyTraceback, int32 MaxDepth = 65536, bool bCaptureVariables = false);
	
	/**
	 * Capture stack frames optimized for performance (lightweight, no variables)
	 * @param StartFrame The starting frame to capture from
	 * @param MaxFrames Maximum number of frames to capture
	 */
	void CaptureStackFramesOptimized(PyFrameObject* StartFrame, int32 MaxFrames);
	
	/**
	 * Update thread-local cache for stack reuse optimization
	 * @param TopFrame The top frame of the stack
	 */
	void UpdateThreadLocalCache(PyFrameObject* TopFrame);

	/**
	 * Get a formatted string representation of the call stack
	 * @param bIncludeVariables Whether to include variable information
	 * @param bIncludeClosures Whether to include closure variables
	 * @return Formatted string
	 */
	FString ToString(bool bIncludeVariables = true, bool bIncludeClosures = true) const;

	/**
	 * Get the number of frames in the call stack
	 */
	int32 GetFrameCount() const { return Frames.Num(); }
	int32 GetDepth() const { return Frames.Num(); }

	/**
	 * Get a specific frame by index
	 */
	const FNePyFrameInfo* GetFrame(int32 Index) const
	{
		if (Index >= 0 && Index < Frames.Num())
		{
			return &Frames[Index];
		}
		return nullptr;
	}

	/**
	 * Get all frames
	 */
	const TArray<FNePyFrameInfo>& GetFrames() const { return Frames; }
	
	/**
	 * Check if stack is empty
	 */
	bool IsEmpty() const { return Frames.Num() == 0; }
	
	/**
	 * Get top function name
	 */
	FString GetTopFunction() const
	{
		return Frames.Num() > 0 ? Frames[0].GetFunctionName() : TEXT("Unknown");
	}
	
	/**
	 * Get hash for the call stack (for deduplication)
	 */
	uint32 GetTypeHash() const
	{
		if (CachedHash == 0)
		{
			CachedHash = ::GetTypeHash(ThreadId);
			for (const FNePyFrameInfo& Frame : Frames)
			{
				CachedHash = HashCombine(CachedHash, Frame.GetTypeHash());
			}
		}
		return CachedHash;
	}
	
	bool operator==(const FNePyCallInfo& Other) const
	{
		return ThreadId == Other.ThreadId && Frames == Other.Frames;
	}
	
	// Thread-local cache for stack reuse optimization
	static thread_local FNePyCallInfo* CachedStack;
	static thread_local uint32 LastFrameCount;
	static thread_local PyFrameObject* LastTopFrame;
	
	/**
	 * Cleanup thread-local cache
	 */
	static void CleanupThreadLocalCache();

private:
	TArray<FNePyFrameInfo> Frames;
	uint8 ThreadId;
	mutable uint32 CachedHash;

	/**
	 * Extract variable information from a Python frame
	 */
	bool ExtractFrameInfo(PyFrameObject* Frame, FNePyFrameInfo& OutFrameInfo);

	/**
	 * Extract local variables from a frame
	 */
	void ExtractLocalVariables(PyFrameObject* Frame, FNePyFrameInfo& OutFrameInfo);

	/**
	 * Extract closure/upvalue variables from a frame
	 * @param Frame The Python frame object
	 * @param Code The code object associated with the frame
	 * @param OutFrameInfo Output structure to store the extracted closure variables
	 */
	void ExtractClosureVariables(PyFrameObject* Frame, PyCodeObject* Code, FNePyFrameInfo& OutFrameInfo);

	/**
	 * Convert a Python object to a string representation
	 */
	FString PyObjectToString(PyObject* Obj, int32 MaxLength = 100);

	/**
	 * Get the type name of a Python object
	 */
	FString GetPyObjectTypeName(PyObject* Obj);
};

inline uint32 GetTypeHash(const FNePyCallInfo& Stack)
{
	return Stack.GetTypeHash();
}

// Legacy alias for backward compatibility
using FNePyCallStack = FNePyCallInfo;

namespace NePyCallInfo
{
	FString GetCurrentCallInfo(bool bIncludeVariables = true, bool bIncludeClosures = true, int32 MaxDepth = 65536);
	FString GetTracebackCallInfo(PyObject* PyTraceback, bool bIncludeVariables = true, bool bIncludeClosures = true, int32 MaxDepth = 65536);
	bool IsGILHeldByOtherThread();
	void PrintCurrentCallInfo();
	void SafePrintCurrentCallInfo();
}