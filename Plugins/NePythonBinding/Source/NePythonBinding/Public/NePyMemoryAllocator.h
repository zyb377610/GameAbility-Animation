// NePyMemoryAllocator.h
#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "HAL/PlatformTLS.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "Misc/DateTime.h"
#include "Logging/LogMacros.h"
#include "NePyBase.h"
#include "NePyCallInfo.h"
#include "frameobject.h"
#include <atomic>

#if ENABLE_LOW_LEVEL_MEM_TRACKER
#include "HAL/LowLevelMemTracker.h"
LLM_DECLARE_TAG_API(NePython, NEPYTHONBINDING_API);
#endif

// Python version compatibility macros
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 4
#define NEPY_HAS_ALLOCATOR_API 1
#include "tracemalloc.h"
#else
#define NEPY_HAS_ALLOCATOR_API 0
#endif

/**
 * Custom allocator function pointers for flexibility
 */
struct FNePyCustomAllocator
{
	typedef void* (*MallocFunc)(size_t Size);
	typedef void* (*CallocFunc)(size_t NumElements, size_t ElementSize);
	typedef void* (*ReallocFunc)(void* Ptr, size_t NewSize);
	typedef void (*FreeFunc)(void* Ptr);

	MallocFunc Malloc;
	CallocFunc Calloc;
	ReallocFunc Realloc;
	FreeFunc Free;

	FNePyCustomAllocator()
		: Malloc(nullptr), Calloc(nullptr), Realloc(nullptr), Free(nullptr)
	{
	}

	FNePyCustomAllocator(MallocFunc InMalloc, CallocFunc InCalloc, ReallocFunc InRealloc, FreeFunc InFree)
		: Malloc(InMalloc), Calloc(InCalloc), Realloc(InRealloc), Free(InFree)
	{
	}
};

/**
 * Information about a Python object
 */
struct FNePyObjectInfo
{
	FString TypeName;
	FString UObjectName;
	uint64 ObjectId;      // Complete object pointer address
	uint8 ObjectFlags;

	enum EObjectFlags : uint8
	{
		FLAG_IS_CONTAINER = 1 << 0,  // list, dict, set, tuple
		FLAG_IS_STRING = 1 << 1,     // str, bytes, unicode
		FLAG_IS_NUMERIC = 1 << 2,    // int, long, float, complex
		FLAG_IS_BUILTIN = 1 << 3,    // builtin type
		FLAG_IS_CALLABLE = 1 << 4,   // function, method, callable
		FLAG_IS_UOBJECT = 1 << 5,    // is a wrapped UObject
	};

	FNePyObjectInfo()
		: ObjectId(0), ObjectFlags(0)
	{
	}

	explicit FNePyObjectInfo(PyObject* Obj);

	bool IsContainer() const { return (ObjectFlags & FLAG_IS_CONTAINER) != 0; }
	bool IsString() const { return (ObjectFlags & FLAG_IS_STRING) != 0; }
	bool IsNumeric() const { return (ObjectFlags & FLAG_IS_NUMERIC) != 0; }
	bool IsBuiltin() const { return (ObjectFlags & FLAG_IS_BUILTIN) != 0; }
	bool IsUObject() const { return (ObjectFlags & FLAG_IS_UOBJECT) != 0; }
};

/**
 * Information about a single memory allocation
 */
struct FNePyAllocationInfo
{
	void* Ptr;
	uint32 Size;
	uint32 AllocTime;     // Compact timestamp
	uint32 StackIndex;    // Index into stack table
	uint8 ThreadId;       // Thread ID (lower 8 bits)
	PyObject* Owner;      // The Python object that owns this allocation

	FNePyAllocationInfo()
		: Ptr(nullptr), Size(0), AllocTime(0), StackIndex(0), ThreadId(0), Owner(nullptr)
	{
	}

	FNePyAllocationInfo(void* InPtr, uint32 InSize, PyObject* InOwner = nullptr);

	double GetAllocTimeSeconds() const;
	bool IsValid() const { return Ptr != nullptr && Size > 0; }
};

/**
 * Top allocator information for statistics
 */
struct FNePyTopAllocator
{
	uint32 StackIndex;
	uint64 TotalSize;
	uint32 Count;
	FString Description;

	FNePyTopAllocator()
		: StackIndex(0), TotalSize(0), Count(0)
	{
	}
};

/**
 * Memory statistics
 */
struct FNePyMemoryStats
{
	std::atomic<uint64> CurrentAllocated{ 0 };
	std::atomic<uint64> PeakAllocated{ 0 };
	std::atomic<uint64> TotalAllocations{ 0 };
	std::atomic<uint32> ActiveAllocations{ 0 };

	void UpdateAllocation(uint32 Size);
	void UpdateDeallocation(uint32 Size);
	void Reset();

	FString GetSummary() const;

	uint64 GetCurrentAllocated() const { return CurrentAllocated.load(); }
	uint64 GetPeakAllocated() const { return PeakAllocated.load(); }
	uint64 GetTotalAllocations() const { return TotalAllocations.load(); }
	uint32 GetActiveAllocations() const { return ActiveAllocations.load(); }
};

/**
 * Configuration for flame graph export
 */
struct FNePyFlameGraphConfig
{
	FString Title = TEXT("Python Memory Allocations");
	uint32 MinSize = 0;           // Minimum allocation size to include
	double MinPercentage = 0.0;   // Minimum percentage to include
	bool bShowObjectTypes = true; // Include object type information
	bool bReverseStack = false;   // Reverse stack order (top-down vs bottom-up)
	int32 MaxStackDepth = 65536;  // Maximum stack depth to include
	bool bOnlyUObjects = false;   // Only include allocations owned by UObjects
};

/**
 * Flame graph exporter
 */
class FNePyFlameGraphExporter
{
public:
	static bool ExportFoldedStacks(
		const TArray<FNePyAllocationInfo>& Allocations,
		const TArray<FNePyCallInfo>& StackTable,
		const FString& FilePath,
		const FNePyFlameGraphConfig& Config = FNePyFlameGraphConfig()
	);

	static bool ExportSVG(
		const TArray<FNePyAllocationInfo>& Allocations,
		const TArray<FNePyCallInfo>& StackTable,
		const FString& FilePath,
		const FNePyFlameGraphConfig& Config = FNePyFlameGraphConfig()
	);

	static bool ExportFoldedStacksDiff(
		const FString& FileBeforePath,
		const FString& FileThenPath,
		const FString& OutputFilePath);

	static bool ExportSVGDiff(
		const FString& FileBeforePath,
		const FString& FileThenPath,
		const FString& OutputFilePath);

	static bool ConvertFoldedToSVG(const FString& FoldedFilePath);
};

/**
 * Main memory allocator class
 */
class FNePyMemoryAllocator
{
public:
	FNePyMemoryAllocator();
	~FNePyMemoryAllocator();

	// Singleton access
	static NEPYTHONBINDING_API FNePyMemoryAllocator& Get();

	// Configuration
	void SetCustomAllocator(const FNePyCustomAllocator& CustomAllocator);

	// Core operations
	bool Initialize();
	void Shutdown();
	bool IsActive() const { return bIsActive; }

	// Statistics and reporting
	const FNePyMemoryStats& GetMemoryStats() const;
	const TArray<FNePyAllocationInfo>& GetActiveAllocations() const;
	const TArray<FNePyCallInfo>& GetStackTable() const;

	// Flame graph export (SVG format)
	bool ExportFlameGraph(const FString& FilePath) const;
	bool ExportFlameGraphWithConfig(const FString& FilePath, const FNePyFlameGraphConfig& Config) const;

	// Folded format (for use with external tools like Brendan Gregg's FlameGraph)
	bool ExportFlameGraphFolded(const FString& FilePath) const;
	bool ExportFlameGraphFoldedWithConfig(const FString& FilePath, const FNePyFlameGraphConfig& Config) const;

	// Report
	bool ExportReport(const FString& FilePath) const;
	FString GenerateDetailedReport() const;
	FString GetObjectFlagsString(uint8 Flags) const;

	// Utility
	void ClearTrackingData();
	static uint32 GetCompactTime();

	// Bind an initialized PyObject to its allocation entry (OBJ domain).
	// Call this after the object is fully constructed (e.g., after PyObject_Init).
	void NEPYTHONBINDING_API BindOwnerIfTracked(PyObject* Obj);
	
private:
	enum class EPyMemoryDomain : uint8
	{
		Raw,
		Mem,
		Obj,
		Unknown
	};

	// Default allocation functions
	static void* DefaultMalloc(size_t Size);
	static void* DefaultCalloc(size_t NumElements, size_t ElementSize);
	static void* DefaultRealloc(void* Ptr, size_t NewSize);
	static void DefaultFree(void* Ptr);

	// Core tracking functions
	void TrackAllocation(void* Ptr, uint32 Size, EPyMemoryDomain Domain);
	void TrackDeallocation(void* Ptr);

	// Stack management
	uint32 GetOrCreateStackIndex(const FNePyCallInfo& Stack);
	const FNePyCallInfo* GetStackByIndex(uint32 Index) const;

	// Python allocator callbacks
#if NEPY_HAS_ALLOCATOR_API
	// Python 3.4+ allocator API
	static void* PyRawMalloc(void* Ctx, size_t Size);
	static void* PyRawCalloc(void* Ctx, size_t NumElements, size_t ElementSize);
	static void* PyRawRealloc(void* Ctx, void* Ptr, size_t NewSize);
	static void PyRawFree(void* Ctx, void* Ptr);

	static void* PyMemMalloc(void* Ctx, size_t Size);
	static void* PyMemCalloc(void* Ctx, size_t NumElements, size_t ElementSize);
	static void* PyMemRealloc(void* Ctx, void* Ptr, size_t NewSize);
	static void PyMemFree(void* Ctx, void* Ptr);

	static void* PyObjMalloc(void* Ctx, size_t Size);
	static void* PyObjCalloc(void* Ctx, size_t NumElements, size_t ElementSize);
	static void* PyObjRealloc(void* Ctx, void* Ptr, size_t NewSize);
	static void PyObjFree(void* Ctx, void* Ptr);
#else
	// Python 2.x and early Python 3.x hook functions
	static void* HookMalloc(size_t Size);
	static void* HookRealloc(void* Ptr, size_t NewSize);
	static void HookFree(void* Ptr);
#endif

	// Member variables
	bool bIsActive;
	double StartTime;
	double LastOptimizationTime;

	// Custom allocator functions
	FNePyCustomAllocator AllocatorFunctions;

	// Data structures
	mutable FCriticalSection CriticalSection;
	TMap<void*, FNePyAllocationInfo> CurrentAllocations;
	TArray<FNePyCallInfo> StackTable;
	TMap<uint32, uint32> StackHashToIndex;

	// Statistics
	FNePyMemoryStats Stats;

	// Original Python allocator functions (for restoration)
#if NEPY_HAS_ALLOCATOR_API
	PyMemAllocatorEx OriginalRawAllocator;
	PyMemAllocatorEx OriginalMemAllocator;
	PyMemAllocatorEx OriginalObjAllocator;
#else
	// Python 2.x function pointers
	void* (*OriginalMalloc)(size_t);
	void* (*OriginalRealloc)(void*, size_t);
	void (*OriginalFree)(void*);
#endif

	// Singleton instance
	static FNePyMemoryAllocator* Instance;

	// Thread-local flag to prevent recursion
	static thread_local bool bInAllocatorCall;
};