// NePyMemoryAllocator.cpp

#include "NePyMemoryAllocator.h"
#include "NePyBase.h"
#include "UObject/UObjectGlobals.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"
#include "Async/TaskGraphInterfaces.h"

#if WITH_EDITOR
#include "Interfaces/IPluginManager.h"
#endif

#if ENABLE_LOW_LEVEL_MEM_TRACKER
LLM_DEFINE_TAG(NePython, "NePython", "NePythonScripting");
#endif

// ===== Global console variables with default values =====

#define NEPY_MEMORY_ALLOCATION_RAW_HOOK_DISABLE 0
#define NEPY_MEMORY_ALLOCATION_MEM_HOOK_DISABLE 0
#define NEPY_MEMORY_ALLOCATION_OBJ_HOOK_DISABLE 0

static bool GNePyMemoryTracking = false;
static int32 GNePyMemoryVerbose = 1;
static int32 GNePyMemoryTopCount = 10;
static int32 GNePyMemoryMaxStack = 65536;

struct FNePyMemoryAllocatorTrackingDisableScope
{
private:
	bool bPreviousState;
public:
	explicit FNePyMemoryAllocatorTrackingDisableScope()
	{
		bPreviousState = GNePyMemoryTracking;
		GNePyMemoryTracking = false;
	}
	~FNePyMemoryAllocatorTrackingDisableScope()
	{
		GNePyMemoryTracking = bPreviousState;
	}
	/** Non-copyable */
	FNePyMemoryAllocatorTrackingDisableScope(const FNePyMemoryAllocatorTrackingDisableScope&) = delete;
	FNePyMemoryAllocatorTrackingDisableScope& operator=(const FNePyMemoryAllocatorTrackingDisableScope&) = delete;
};

#define NEPY_CONCAT_IMPL(A, B) A##B
#define NEPY_CONCAT(A, B) NEPY_CONCAT_IMPL(A, B)
#define NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE \
	FNePyMemoryAllocatorTrackingDisableScope NEPY_CONCAT(DisableTracking, __LINE__); \
	FNePyScopedGIL NEPY_CONCAT(GIL, __LINE__);

// Console variable references bound to global variables
static FAutoConsoleVariableRef CVarNePyMemoryTracking(
	TEXT("nepy.memory.tracking"),
	GNePyMemoryTracking,
	TEXT("Enable/disable Python memory tracking")
);

static FAutoConsoleVariableRef CVarNePyMemoryVerbose(
	TEXT("nepy.memory.verbose"),
	GNePyMemoryVerbose,
	TEXT("Memory stats verbosity level (0=basic, 1=detailed, 2=full)")
);

static FAutoConsoleVariableRef CVarNePyMemoryTopCount(
	TEXT("nepy.memory.topcount"),
	GNePyMemoryTopCount,
	TEXT("Number of top allocators to show")
);

static FAutoConsoleVariableRef CVarNePyMemoryMaxStack(
	TEXT("nepy.memory.maxstack"),
	GNePyMemoryMaxStack,
	TEXT("Maximum stack depth for memory tracking")
);

/**
 * Console commands implementation for NePy memory system
 */
class FNePyMemoryCommands
{
public:
	static void RegisterCommands()
	{
		// Basic memory status
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.status"),
			TEXT("Show Python memory allocation status"),
			FConsoleCommandDelegate::CreateStatic(&FNePyMemoryCommands::ShowStatus)
		);

		// Detailed statistics
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.stats"),
			TEXT("Show detailed Python memory statistics"),
			FConsoleCommandDelegate::CreateStatic(&FNePyMemoryCommands::ShowStats)
		);

		// Top allocators
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.top"),
			TEXT("Show top Python memory allocators [count]"),
			FConsoleCommandWithArgsDelegate::CreateStatic(&FNePyMemoryCommands::ShowTopAllocators)
		);

		// Export report
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.export"),
			TEXT("Export memory report to file [filename]"),
			FConsoleCommandWithArgsDelegate::CreateStatic(&FNePyMemoryCommands::ExportReport)
		);

		// Clear tracking data
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.clear"),
			TEXT("Clear memory tracking data"),
			FConsoleCommandDelegate::CreateStatic(&FNePyMemoryCommands::ClearData)
		);

		// Flame graph export (SVG)
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.flamegraph"),
			TEXT("Export flame graph data [filename]"),
			FConsoleCommandWithArgsDelegate::CreateStatic(&FNePyMemoryCommands::ExportFlameGraph)
		);

		// Flame graph diff export (SVG)
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.flamegraph.diff"),
			TEXT("Export flame graph diff data [filename]"),
			FConsoleCommandWithArgsDelegate::CreateStatic(&FNePyMemoryCommands::ExportFlameGraphDiff)
		);

		// Folded data export
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.flamegraph.folded"),
			TEXT("Export raw folded stack data [filename.folded]"),
			FConsoleCommandWithArgsDelegate::CreateStatic(&FNePyMemoryCommands::ExportFlameGraphFolded)
		);

		// Folded diff data export
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.flamegraph.folded.diff"),
			TEXT("Export raw folded stack diff data [filename.folded]"),
			FConsoleCommandWithArgsDelegate::CreateStatic(&FNePyMemoryCommands::ExportFlameGraphFoldedDiff)
		);

		// Convert folded to SVG
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.flamegraph.svg"),
			TEXT("Convert a .folded file to a flamegraph .svg file [filename.folded]"),
			FConsoleCommandWithArgsDelegate::CreateStatic(&FNePyMemoryCommands::ExportFlameGraphSvg)
		);

		// Open flamegraph location
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.flamegraph.location"),
			TEXT("Open the flamegraph output directory in file explorer"),
			FConsoleCommandDelegate::CreateStatic(&FNePyMemoryCommands::OpenFlameGraphLocation)
		);

		// Configuration commands
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.config"),
			TEXT("Show current configuration"),
			FConsoleCommandDelegate::CreateStatic(&FNePyMemoryCommands::ShowConfig)
		);

		// Help command
		IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("nepy.memory.help"),
			TEXT("Show NePy memory commands help"),
			FConsoleCommandDelegate::CreateStatic(&FNePyMemoryCommands::ShowHelp)
		);
	}

	static void UnregisterCommands()
	{
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.status"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.stats"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.top"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.export"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.clear"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.flamegraph"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.flamegraph.diff"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.flamegraph.folded"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.flamegraph.folded.diff"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.flamegraph.svg"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.flamegraph.location"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.config"));
		IConsoleManager::Get().UnregisterConsoleObject(TEXT("nepy.memory.help"));
	}

private:
	// Show basic memory status
	static void ShowStatus()
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		auto& Allocator = FNePyMemoryAllocator::Get();
		if (!Allocator.IsActive())
		{
			UE_LOG(LogNePython, Warning, TEXT("NePy Memory Tracker is not active"));
			return;
		}

		const auto& Stats = Allocator.GetMemoryStats();

		UE_LOG(LogNePython, Log, TEXT("=== NePy Memory Status ==="));
		UE_LOG(LogNePython, Log, TEXT("Tracking Enabled: %s"), GNePyMemoryTracking ? TEXT("Yes") : TEXT("No"));
		UE_LOG(LogNePython, Log, TEXT("Current Allocated: %.2f MB (%llu bytes)"),
			Stats.GetCurrentAllocated() / (1024.0 * 1024.0), Stats.GetCurrentAllocated());
		UE_LOG(LogNePython, Log, TEXT("Peak Allocated: %.2f MB (%llu bytes)"),
			Stats.GetPeakAllocated() / (1024.0 * 1024.0), Stats.GetPeakAllocated());
		UE_LOG(LogNePython, Log, TEXT("Active Allocations: %u"), Stats.GetActiveAllocations());
		UE_LOG(LogNePython, Log, TEXT("Total Allocations: %llu"), Stats.GetTotalAllocations());
	}

	// Show detailed statistics
	static void ShowStats()
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		auto& Allocator = FNePyMemoryAllocator::Get();
		if (!Allocator.IsActive())
		{
			UE_LOG(LogNePython, Warning, TEXT("NePy Memory Tracker is not active"));
			return;
		}

		const FNePyMemoryStats& Stats = Allocator.GetMemoryStats();
		int32 VerboseLevel = GNePyMemoryVerbose;

		UE_LOG(LogNePython, Log, TEXT("=== NePy Detailed Statistics ==="));

		// Basic information
		ShowStatus();

		const auto& StackTable = Allocator.GetStackTable();
		const auto& CurrentAllocations = Allocator.GetActiveAllocations();

		if (VerboseLevel >= 1)
		{
			// Allocations by type
			UE_LOG(LogNePython, Log, TEXT("\n--- Allocations by Type ---"));

			TMap<FString, uint64> AllocationsByType;
			TMap<FString, uint32> CountsByType;

			for (const auto& Alloc : CurrentAllocations)
			{
				FNePyObjectInfo ObjectInfo(Alloc.Owner);
				FString TypeName = ObjectInfo.TypeName.IsEmpty() ? TEXT("Unknown") : ObjectInfo.TypeName;
				AllocationsByType.FindOrAdd(TypeName) += Alloc.Size;
				CountsByType.FindOrAdd(TypeName)++;
			}

			TArray<TPair<FString, uint64>> SortedBySize;
			for (const auto& Pair : AllocationsByType)
			{
				SortedBySize.Add(Pair);
			}
			SortedBySize.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });

			uint64 TotalAllocatedSize = Stats.GetCurrentAllocated();
			for (const auto& Pair : SortedBySize)
			{
				uint32 Count = CountsByType.Contains(Pair.Key) ? CountsByType[Pair.Key] : 0;
				double Percentage = TotalAllocatedSize > 0 ?
					(double)Pair.Value / TotalAllocatedSize * 100.0 : 0.0;
				UE_LOG(LogNePython, Log, TEXT("  %s: %.2f MB (%.1f%%) - %u allocs"),
					*Pair.Key, Pair.Value / (1024.0 * 1024.0), Percentage, Count);
			}
		}

		if (VerboseLevel >= 2)
		{
			// Stack table information
			UE_LOG(LogNePython, Log, TEXT("\n--- Stack Table Info ---"));
			UE_LOG(LogNePython, Log, TEXT("Total Unique Stacks: %d"), StackTable.Num());

			// Active allocations by stack
			UE_LOG(LogNePython, Log, TEXT("\n--- Active Allocations by Stack ---"));
			TMap<uint32, TArray<const FNePyAllocationInfo*>> ByStack;

			for (const auto& Alloc : CurrentAllocations)
			{
				ByStack.FindOrAdd(Alloc.StackIndex).Add(&Alloc);
			}

			int32 ShowCount = FMath::Min(10, ByStack.Num());
			TArray<TPair<uint32, TArray<const FNePyAllocationInfo*>>> Sorted;
			for (const auto& Pair : ByStack)
			{
				Sorted.Add(Pair);
			}
			Sorted.Sort([](const auto& A, const auto& B) { return A.Value.Num() > B.Value.Num(); });

			for (int32 i = 0; i < ShowCount; ++i)
			{
				const auto& Pair = Sorted[i];
				uint64 TotalSize = 0;
				for (const auto* Alloc : Pair.Value)
				{
					TotalSize += Alloc->Size;
				}

				FString StackDesc = TEXT("Unknown");
				if (Pair.Key < (uint32)StackTable.Num())
				{
					StackDesc = StackTable[Pair.Key].GetTopFunction();
				}

				UE_LOG(LogNePython, Log, TEXT("  [%d] %s: %d allocs, %.1f KB"),
					i + 1, *StackDesc, Pair.Value.Num(), TotalSize / 1024.0);
			}
		}
	}

	// Show top allocators
	static void ShowTopAllocators(const TArray<FString>& Args)
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		auto& Allocator = FNePyMemoryAllocator::Get();
		if (!Allocator.IsActive())
		{
			UE_LOG(LogNePython, Warning, TEXT("NePy Memory Tracker is not active"));
			return;
		}

		int32 TopCount = GNePyMemoryTopCount;
		if (Args.Num() > 0)
		{
			TopCount = FCString::Atoi(*Args[0]);
			TopCount = FMath::Clamp(TopCount, 1, 100);
		}

		const auto& StackTable = Allocator.GetStackTable();
		const auto& Allocations = Allocator.GetActiveAllocations();

		// Group allocations by call stack
		TMap<uint32, TArray<const FNePyAllocationInfo*>> AllocsByStack;
		for (const auto& Alloc : Allocations)
		{
			AllocsByStack.FindOrAdd(Alloc.StackIndex).Add(&Alloc);
		}

		struct FTopAllocator
		{
			uint32 StackIndex;
			uint32 Count;
			uint64 TotalSize;
			FString Description;
		};

		TArray<FTopAllocator> TopAllocators;
		for (const auto& Pair : AllocsByStack)
		{
			FTopAllocator Top;
			Top.StackIndex = Pair.Key;
			Top.Count = Pair.Value.Num();
			Top.TotalSize = 0;
			for (const auto* Alloc : Pair.Value)
				Top.TotalSize += Alloc->Size;
			Top.Description = (Top.StackIndex < (uint32)StackTable.Num()) ? StackTable[Top.StackIndex].GetTopFunction() : TEXT("Unknown");
			TopAllocators.Add(Top);
		}
		TopAllocators.Sort([](const FTopAllocator& A, const FTopAllocator& B) { return A.TotalSize > B.TotalSize; });

		UE_LOG(LogNePython, Log, TEXT("=== Top %d Python Allocators ==="), TopCount);
		for (int32 i = 0; i < FMath::Min(TopCount, TopAllocators.Num()); ++i)
		{
			const auto& Top = TopAllocators[i];
			double Percentage = Allocator.GetMemoryStats().GetCurrentAllocated() > 0 ?
				(double)Top.TotalSize / Allocator.GetMemoryStats().GetCurrentAllocated() * 100.0 : 0.0;
			UE_LOG(LogNePython, Log, TEXT("[%d] %.2f MB (%.1f%%) - %u allocs - %s"),
				i + 1, Top.TotalSize / (1024.0 * 1024.0), Percentage, Top.Count, *Top.Description);

			// Show up to 5 sample allocations for this stack
			const auto& AllocsForStack = AllocsByStack[Top.StackIndex];
			int32 SampleCount = FMath::Min(5, AllocsForStack.Num());
			for (int32 j = 0; j < SampleCount; ++j)
			{
				const auto* Alloc = AllocsForStack[j];
				FNePyObjectInfo ObjectInfo(Alloc->Owner);
				double Age = Alloc->GetAllocTimeSeconds();
				FString TypeName = ObjectInfo.TypeName.IsEmpty() ?
					TEXT("Unknown") : ObjectInfo.TypeName;

				FString UObjectStr;
				if (!ObjectInfo.UObjectName.IsEmpty())
				{
					UObjectStr = FString::Printf(TEXT(", uobject: %s"), *ObjectInfo.UObjectName);
				}

				UE_LOG(LogNePython, VeryVerbose, TEXT("  - %u bytes, age: %.1f sec, type: %s, thread: %u%s"),
					Alloc->Size, Age, *TypeName, Alloc->ThreadId, *UObjectStr);
			}
		}
	}

	// Export detailed report
	static void ExportReport(const TArray<FString>& Args)
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		auto& Allocator = FNePyMemoryAllocator::Get();
		if (!Allocator.IsActive())
		{
			UE_LOG(LogNePython, Warning, TEXT("NePy Memory Tracker is not active"));
			return;
		}

		FString FileName = TEXT("nepy_memory_report.txt");
		if (Args.Num() > 0)
		{
			FileName = Args[0];
			if (!FileName.EndsWith(TEXT(".txt")))
			{
				FileName += TEXT(".txt");
			}
		}

		FString ProjectSavedDir = FPaths::ProjectSavedDir();
		FString PythonDir = ProjectSavedDir / TEXT("Python");
		FString FilePath = PythonDir / FileName;

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*PythonDir))
		{
			PlatformFile.CreateDirectoryTree(*PythonDir);
		}

		if (Allocator.ExportReport(FilePath))
		{
			UE_LOG(LogNePython, Log, TEXT("Memory report exported to: %s"), *FPaths::ConvertRelativePathToFull(FilePath));
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Failed to export memory report to: %s"), *FPaths::ConvertRelativePathToFull(FilePath));
		}
	}

	// Clear tracking data
	static void ClearData()
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		auto& Allocator = FNePyMemoryAllocator::Get();
		if (!Allocator.IsActive())
		{
			UE_LOG(LogNePython, Warning, TEXT("NePy Memory Tracker is not active"));
			return;
		}

		Allocator.ClearTrackingData();
		UE_LOG(LogNePython, Log, TEXT("NePy memory tracking data cleared"));
	}

	// Export raw folded flame graph data (SVG)
	static void ExportFlameGraph(const TArray<FString>& Args)
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		auto& Allocator = FNePyMemoryAllocator::Get();
		if (!Allocator.IsActive())
		{
			UE_LOG(LogNePython, Warning, TEXT("NePy Memory Tracker is not active"));
			return;
		}

		FString FileName = TEXT("nepy_flamegraph.svg");
		bool bOnlyUObjects = false;

		for (const FString& Arg : Args)
		{
			if (Arg.StartsWith(TEXT("-")))
			{
				if (Arg == TEXT("-uobjects"))
				{
					bOnlyUObjects = true;
				}
			}
			else
			{
				FileName = Arg;
			}
		}

		if (!FileName.EndsWith(TEXT(".svg"), ESearchCase::IgnoreCase))
		{
			FileName = FileName + TEXT(".svg");
		}

		FString ProjectSavedDir = FPaths::ProjectSavedDir();
		FString PythonDir = ProjectSavedDir / TEXT("Python");
		FString FilePath = PythonDir / FileName;

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*PythonDir))
		{
			PlatformFile.CreateDirectoryTree(*PythonDir);
		}

		FNePyFlameGraphConfig Config;
		Config.bOnlyUObjects = bOnlyUObjects;
		if (bOnlyUObjects)
		{
			Config.Title = TEXT("Python Memory Allocations (UObjects Only)");
		}

		if (Allocator.ExportFlameGraphWithConfig(FilePath, Config))
		{
			UE_LOG(LogNePython, Log, TEXT("Folded flame graph data exported to: %s"), *FPaths::ConvertRelativePathToFull(FilePath));
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Failed to export folded flame graph data to: %s"), *FPaths::ConvertRelativePathToFull(FilePath));
		}
	}

	static void ExportFlameGraphFolded(const TArray<FString>& Args)
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		auto& Allocator = FNePyMemoryAllocator::Get();
		if (!Allocator.IsActive())
		{
			UE_LOG(LogNePython, Warning, TEXT("NePy Memory Tracker is not active"));
			return;
		}

		FString FileName = TEXT("nepy_flamegraph.folded");
		bool bOnlyUObjects = false;

		for (const FString& Arg : Args)
		{
			if (Arg.StartsWith(TEXT("-")))
			{
				if (Arg == TEXT("-uobjects"))
				{
					bOnlyUObjects = true;
				}
			}
			else
			{
				FileName = Arg;
			}
		}

		if (!FileName.EndsWith(TEXT(".folded"), ESearchCase::IgnoreCase))
		{
			FileName = FileName + TEXT(".folded");
		}

		FString ProjectSavedDir = FPaths::ProjectSavedDir();
		FString PythonDir = ProjectSavedDir / TEXT("Python");
		FString FilePath = PythonDir / FileName;

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*PythonDir))
		{
			PlatformFile.CreateDirectoryTree(*PythonDir);
		}

		FNePyFlameGraphConfig Config;
		Config.bOnlyUObjects = bOnlyUObjects;
		if (bOnlyUObjects)
		{
			Config.Title = TEXT("Python Memory Allocations (UObjects Only)");
		}

		if (Allocator.ExportFlameGraphFoldedWithConfig(FilePath, Config))
		{
			UE_LOG(LogNePython, Log, TEXT("Folded flame graph data exported to: %s"), *FPaths::ConvertRelativePathToFull(FilePath));
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Failed to export folded flame graph data to: %s"), *FPaths::ConvertRelativePathToFull(FilePath));
		}
	}

	static void ExportFlameGraphDiff(const TArray<FString>& Args)
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		if (Args.Num() < 3)
		{
			UE_LOG(LogNePython, Error, TEXT("Usage: nepy.memory.flamegraph.diff <file_before.folded> <file_then.folded> <output_file.svg>"));
			return;
		}

		auto& Allocator = FNePyMemoryAllocator::Get();
		FString FileBefore = Args[0];
		FString FileThen = Args[1];
		FString OutputFile = Args[2];

		if (!FileBefore.EndsWith(TEXT(".folded"), ESearchCase::IgnoreCase))
		{
			FileBefore = FileBefore + TEXT(".folded");
		}

		if (!FileThen.EndsWith(TEXT(".folded"), ESearchCase::IgnoreCase))
		{
			FileThen = FileThen + TEXT(".folded");
		}

		if (!OutputFile.EndsWith(TEXT(".svg"), ESearchCase::IgnoreCase))
		{
			OutputFile = OutputFile + TEXT(".svg");
		}

		FString ProjectSavedDir = FPaths::ProjectSavedDir();
		FString PythonDir = ProjectSavedDir / TEXT("Python");

		FString FileBeforePath = PythonDir / FileBefore;
		FString FileThenPath = PythonDir / FileThen;
		FString OutputFilePath = PythonDir / OutputFile;

		if (FNePyFlameGraphExporter::ExportSVGDiff(FileBeforePath, FileThenPath, OutputFilePath))
		{
			UE_LOG(LogNePython, Log, TEXT("Flame graph diff exported to: %s"), *FPaths::ConvertRelativePathToFull(OutputFilePath));
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Failed to export flame graph diff to: %s"), *FPaths::ConvertRelativePathToFull(OutputFilePath));
		}
	}

	static void ExportFlameGraphFoldedDiff(const TArray<FString>& Args)
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		if (Args.Num() < 3)
		{
			UE_LOG(LogNePython, Error, TEXT("Usage: nepy.memory.flamegraph.folded.diff <file_before.folded> <file_then.folded> <output_file.folded>"));
			return;
		}

		auto& Allocator = FNePyMemoryAllocator::Get();
		FString FileBefore = Args[0];
		FString FileThen = Args[1];
		FString OutputFile = Args[2];

		if (!FileBefore.EndsWith(TEXT(".folded"), ESearchCase::IgnoreCase))
		{
			FileBefore = FileBefore + TEXT(".folded");
		}

		if (!FileThen.EndsWith(TEXT(".folded"), ESearchCase::IgnoreCase))
		{
			FileThen = FileThen + TEXT(".folded");
		}

		if (!OutputFile.EndsWith(TEXT(".folded"), ESearchCase::IgnoreCase))
		{
			OutputFile = OutputFile + TEXT(".folded");
		}

		FString ProjectSavedDir = FPaths::ProjectSavedDir();
		FString PythonDir = ProjectSavedDir / TEXT("Python");

		FString FileBeforePath = PythonDir / FileBefore;
		FString FileThenPath = PythonDir / FileThen;
		FString OutputFilePath = PythonDir / OutputFile;

		if (FNePyFlameGraphExporter::ExportFoldedStacksDiff(FileBeforePath, FileThenPath, OutputFilePath))
		{
			UE_LOG(LogNePython, Log, TEXT("Folded flame graph diff exported to: %s"), *FPaths::ConvertRelativePathToFull(OutputFilePath));
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Failed to export folded flame graph diff to: %s"), *FPaths::ConvertRelativePathToFull(OutputFilePath));
		}
	}

	static void ExportFlameGraphSvg(const TArray<FString>& Args)
	{
		NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

		if (Args.Num() < 1)
		{
			UE_LOG(LogNePython, Error, TEXT("Usage: nepy.memory.flamegraph.svg <file.folded>"));
			return;
		}

		FString FoldedFile = Args[0];
		if (!FoldedFile.EndsWith(TEXT(".folded"), ESearchCase::IgnoreCase))
		{
			FoldedFile += TEXT(".folded");
		}

		FString ProjectSavedDir = FPaths::ProjectSavedDir();
		FString PythonDir = ProjectSavedDir / TEXT("Python");
		FString FoldedFilePath = PythonDir / FoldedFile;

		if (!FPaths::FileExists(FoldedFilePath))
		{
			UE_LOG(LogNePython, Error, TEXT("Folded file not found: %s"), *FPaths::ConvertRelativePathToFull(FoldedFilePath));
			return;
		}

		if (FNePyFlameGraphExporter::ConvertFoldedToSVG(FoldedFilePath))
		{
			UE_LOG(LogNePython, Log, TEXT("Flame graph SVG conversion successful for: %s"), *FPaths::ConvertRelativePathToFull(FoldedFilePath));
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Failed to convert folded file to SVG: %s"), *FPaths::ConvertRelativePathToFull(FoldedFilePath));
		}
	}

	static void OpenFlameGraphLocation()
	{
		FString ProjectSavedDir = FPaths::ProjectSavedDir();
		FString PythonDir = ProjectSavedDir / TEXT("Python");
		PythonDir = FPaths::ConvertRelativePathToFull(PythonDir);
#if WITH_EDITOR
		if (!FPaths::DirectoryExists(PythonDir))
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			PlatformFile.CreateDirectoryTree(*PythonDir);
		}
		FPlatformProcess::LaunchFileInDefaultExternalApplication(*PythonDir);
		UE_LOG(LogNePython, Log, TEXT("Opened flamegraph location: %s"), *PythonDir);
#else
		UE_LOG(LogNePython, Display, TEXT("Flamegraph location: %s"), *PythonDir);
#endif
	}

	// Show current configuration
	static void ShowConfig()
	{
		UE_LOG(LogNePython, Log, TEXT("=== NePy Memory Configuration ==="));
		UE_LOG(LogNePython, Log, TEXT("Tracking Enabled: %s"), GNePyMemoryTracking ? TEXT("Yes") : TEXT("No"));
		UE_LOG(LogNePython, Log, TEXT("Verbose Level: %d"), GNePyMemoryVerbose);
		UE_LOG(LogNePython, Log, TEXT("Top Count: %d"), GNePyMemoryTopCount);
	}

	// Show help information
	static void ShowHelp()
	{
		UE_LOG(LogNePython, Log, TEXT("=== NePy Memory Commands Help ==="));
		UE_LOG(LogNePython, Log, TEXT("Commands:"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.status                - Show memory status"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.stats                 - Show detailed statistics"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.top [count]           - Show top allocators"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.export [file]         - Export report"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.clear                 - Clear tracking data"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.config                - Show configuration"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.help                  - Show this help message"));
		UE_LOG(LogNePython, Log, TEXT(""));
		UE_LOG(LogNePython, Log, TEXT("Flamegraph Commands:"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.flamegraph [file] [-uobjects] - Export flame graph SVG. Use -uobjects to only include UObject allocations."));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.flamegraph.folded [file] [-uobjects] - Export flame graph folded data. Use -uobjects to only include UObject allocations."));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.flamegraph.svg <file.folded> - Convert a .folded file to a flamegraph .svg file."));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.flamegraph.location - Open the flamegraph output directory in file explorer."));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.flamegraph.diff <before.folded> <then.folded> <out.svg> - Export flame graph diff SVG from two folded files."));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.flamegraph.folded.diff <before.folded> <then.folded> <out.folded> - Export folded stack diff from two folded files."));
		UE_LOG(LogNePython, Log, TEXT(""));
		UE_LOG(LogNePython, Log, TEXT("Console Variables:"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.tracking              - Enable/disable tracking (Default: false)"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.verbose               - Verbosity level (0-2) (Default: 1)"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.topcount              - Number of top items to show (Default: 10)"));
		UE_LOG(LogNePython, Log, TEXT("  nepy.memory.maxstack              - Maximum stack depth for tracking (Default: 65536)"));
	}
};

// ===== FNePyObjectInfo Implementation =====

FNePyObjectInfo::FNePyObjectInfo(PyObject* Obj)
	: ObjectId(0), ObjectFlags(0)
{
	if (!Obj)
	{
		return;
	}

	// Store complete pointer address
	ObjectId = reinterpret_cast<uint64>(Obj);

	// Get type name
	if (Obj->ob_type && Obj->ob_type->tp_name)
	{
		TypeName = FString(UTF8_TO_TCHAR(Obj->ob_type->tp_name));
	}

	// Check if this Python object wraps a UObject
	if (UObject* UEObject = NePyBase::ToCppObject(Obj))
	{
		UObjectName = UEObject->GetFullName();
		ObjectFlags |= FLAG_IS_UOBJECT;
	}

	// Set object flags based on type
	if (PyList_Check(Obj) || PyDict_Check(Obj) || PySet_Check(Obj) || PyTuple_Check(Obj))
	{
		ObjectFlags |= FLAG_IS_CONTAINER;
	}

	if (PyUnicode_Check(Obj) || PyBytes_Check(Obj))
	{
		ObjectFlags |= FLAG_IS_STRING;
	}

	if (PyLong_Check(Obj) || PyFloat_Check(Obj) || PyComplex_Check(Obj))
	{
		ObjectFlags |= FLAG_IS_NUMERIC;
	}

	if (PyType_Check(Obj) && PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(Obj), &PyType_Type))
	{
		ObjectFlags |= FLAG_IS_BUILTIN;
	}

	if (PyCallable_Check(Obj))
	{
		ObjectFlags |= FLAG_IS_CALLABLE;
	}
}

// ===== FNePyAllocationInfo Implementation =====

FNePyAllocationInfo::FNePyAllocationInfo(void* InPtr, uint32 InSize, PyObject* InOwner)
	: Ptr(InPtr), Size(InSize), AllocTime(0), StackIndex(0), ThreadId(0), Owner(InOwner)
{
	AllocTime = FNePyMemoryAllocator::GetCompactTime();
	ThreadId = (uint8)(FPlatformTLS::GetCurrentThreadId() & 0xFF);
}

double FNePyAllocationInfo::GetAllocTimeSeconds() const
{
	// Convert compact time back to seconds since start
	return (double)AllocTime * 0.1; // 100ms resolution
}

// ===== FNePyMemoryStats Implementation =====

void FNePyMemoryStats::UpdateAllocation(uint32 Size)
{
	uint64 NewCurrent = CurrentAllocated.fetch_add(Size) + Size;
	uint64 CurrentPeak = PeakAllocated.load();
	while (NewCurrent > CurrentPeak && !PeakAllocated.compare_exchange_weak(CurrentPeak, NewCurrent))
	{
		// Retry until we successfully update the peak or it's no longer needed
	}

	TotalAllocations.fetch_add(1);
	ActiveAllocations.fetch_add(1);
}

void FNePyMemoryStats::UpdateDeallocation(uint32 Size)
{
	CurrentAllocated.fetch_sub(Size);
	ActiveAllocations.fetch_sub(1);
}

void FNePyMemoryStats::Reset()
{
	CurrentAllocated = 0;
	PeakAllocated = 0;
	TotalAllocations = 0;
	ActiveAllocations = 0;
}

FString FNePyMemoryStats::GetSummary() const
{
	return FString::Printf(
		TEXT("Current: %llu bytes, Peak: %llu bytes, Total Allocs: %llu, Active: %u"),
		GetCurrentAllocated(),
		GetPeakAllocated(),
		GetTotalAllocations(),
		GetActiveAllocations()
	);
}

// ===== FNePyFlameGraphExporter Implementation =====

// Helper: build a compact owner label and sanitize forbidden characters for folded format.
static inline FString MakeOwnerLabel(const FNePyObjectInfo& Info, bool bIncludeType, bool bIncludeAddress, bool bIncludeUObject)
{
	FString Label;

	// Include Python type name
	if (bIncludeType && !Info.TypeName.IsEmpty())
	{
		Label += Info.TypeName;
	}

	// Include UObject name (if available in your build; Info.UObjectName may be empty)
	if (bIncludeUObject && !Info.UObjectName.IsEmpty())
	{
		if (!Label.IsEmpty()) Label += TEXT("|");
		Label += Info.UObjectName;
	}

	// Include object address to distinguish instances
	if (bIncludeAddress && Info.ObjectId != 0)
	{
		if (!Label.IsEmpty()) Label += TEXT("@");
		Label += FString::Printf(TEXT("%p"), reinterpret_cast<void*>(Info.ObjectId));
	}

	// Sanitize: folded format uses ';' as a separator and newlines as record separators
	Label.ReplaceInline(TEXT(";"), TEXT(":"));
	Label.ReplaceInline(TEXT("\n"), TEXT(" "));
	Label.ReplaceInline(TEXT("\r"), TEXT(" "));

	return Label;
}

bool FNePyFlameGraphExporter::ExportFoldedStacks(
	const TArray<FNePyAllocationInfo>& Allocations,
	const TArray<FNePyCallStack>& StackTable,
	const FString& FilePath,
	const FNePyFlameGraphConfig& Config)
{
	TMap<FString, uint64> FoldedStacks;
	uint64 TotalSize = 0;

	for (const FNePyAllocationInfo& Alloc : Allocations)
	{
		if (Alloc.Size < Config.MinSize) continue;

		const FNePyObjectInfo ObjectInfo(Alloc.Owner);
		if (Config.bOnlyUObjects && !ObjectInfo.IsUObject())
		{
			continue;
		}

		FString StackString;
		if (Alloc.StackIndex < (uint32)StackTable.Num())
		{
			const FNePyCallStack& Stack = StackTable[Alloc.StackIndex];

			// Build an optional owner label once per allocation
			FString OwnerLabel;
			if (Config.bShowObjectTypes && Alloc.Owner != nullptr)
			{
				// Configure what to include in the label:
				// - Type name: true
				// - Address: true (helps distinguish instances; set false to aggregate by type)
				// - UObject name: true (enable if you populate UObjectName in FNePyObjectInfo)
				OwnerLabel = MakeOwnerLabel(
					ObjectInfo,
					/*bIncludeType=*/true,
					/*bIncludeAddress=*/true,
					/*bIncludeUObject=*/true
				);

				if (!OwnerLabel.IsEmpty())
				{
					OwnerLabel = FString::Printf(TEXT("<%s>"), *OwnerLabel);
				}
			}

			// Build folded stack; attach owner only to the leaf (last) frame to avoid explosion
			TArray<FString> StackParts;
			const TArray<FNePyStackFrame>& StackFrames = Stack.GetFrames();
			const int32 Depth = Stack.GetDepth();
			const int32 MaxDepth = FMath::Min(Depth, Config.MaxStackDepth);

			for (int32 i = 0; i < MaxDepth; ++i)
			{
				const int32 FrameIndex = Config.bReverseStack ? i : (MaxDepth - 1 - i);
				const FNePyStackFrame& Frame = StackFrames[FrameIndex];

				FString FrameString = Frame.ToFoldedString();

				// Leaf is the last segment we append (i == MaxDepth - 1) in the final folded string
				const bool bIsLeaf = (i == MaxDepth - 1);
				if (bIsLeaf && !OwnerLabel.IsEmpty())
				{
					FrameString += OwnerLabel;
				}

				StackParts.Add(MoveTemp(FrameString));
			}

			StackString = FString::Join(StackParts, TEXT(";"));
		}
		else
		{
			StackString = TEXT("Unknown");
		}

		FoldedStacks.FindOrAdd(StackString) += Alloc.Size;
		TotalSize += Alloc.Size;
	}

	const double MinBytes = TotalSize * Config.MinPercentage / 100.0;

	FString Output;
	Output += FString::Printf(TEXT("# %s\n"), *Config.Title);
	Output += FString::Printf(TEXT("# Total: %llu bytes\n"), TotalSize);

	for (const auto& Entry : FoldedStacks)
	{
		if (Entry.Value >= MinBytes)
		{
			Output += FString::Printf(TEXT("%s %llu\n"), *Entry.Key, Entry.Value);
		}
	}

	return FFileHelper::SaveStringToFile(Output, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

static FString GetPythonExecutablePath()
{
	static FString CachedPythonExecutablePath;
	if (!CachedPythonExecutablePath.IsEmpty())
	{
		return CachedPythonExecutablePath;
	}

	// Search the system's PATH.
	const FString PathVariable = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
	TArray<FString> PathDirectories;
	PathVariable.ParseIntoArray(PathDirectories, FPlatformMisc::GetPathVarDelimiter());

	TArray<FString> ExecutableNames;
#if PLATFORM_WINDOWS
	ExecutableNames.Add(TEXT("python3.exe"));
	ExecutableNames.Add(TEXT("python.exe"));
#else
	ExecutableNames.Add(TEXT("python3"));
	ExecutableNames.Add(TEXT("python"));
#endif

	for (const FString& ExeName : ExecutableNames)
	{
		for (const FString& Dir : PathDirectories)
		{
			FString FullPath = FPaths::Combine(Dir, ExeName);
			if (FPaths::FileExists(FullPath))
			{
				UE_LOG(LogNePython, Log, TEXT("Found Python executable in PATH: %s"), *FullPath);
				CachedPythonExecutablePath = FullPath;
				return CachedPythonExecutablePath;
			}
		}
	}

	// If still not found, log a warning and return the first fallback name.
	const FString FallbackExe = ExecutableNames.Num() > 0 ? ExecutableNames[0] : TEXT("python");
	UE_LOG(LogNePython, Warning, TEXT("Could not find Python executable in system PATH. Falling back to '%s'. Please ensure Python is installed and in the PATH."), *FallbackExe);
	CachedPythonExecutablePath = FallbackExe;
	return CachedPythonExecutablePath;
}

bool FNePyFlameGraphExporter::ExportSVG(
	const TArray<FNePyAllocationInfo>& Allocations,
	const TArray<FNePyCallStack>& StackTable,
	const FString& FilePath,
	const FNePyFlameGraphConfig& Config)
{
#if WITH_EDITOR
	FString FoldedFilePath = FPaths::ConvertRelativePathToFull(FPaths::ChangeExtension(FilePath, TEXT(".folded")));

	if (!ExportFoldedStacks(Allocations, StackTable, FoldedFilePath, Config))
	{
		UE_LOG(LogNePython, Error, TEXT("Failed to export temporary folded stack file to: %s"), *FoldedFilePath);
		return false;
	}

	return ConvertFoldedToSVG(FoldedFilePath);
#else
	UE_LOG(LogNePython, Error, TEXT("Flamegraph export is only available in the editor."));
	return false;
#endif
}

bool FNePyFlameGraphExporter::ConvertFoldedToSVG(const FString& FoldedFilePath)
{
#if WITH_EDITOR
	// 1. Locate the flamegraph generation script within the plugin
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("NePythonBinding"));
	if (!Plugin.IsValid())
	{
		UE_LOG(LogNePython, Error, TEXT("Could not find NePythonBinding plugin to locate flamegraph script."));
		return false;
	}
	const FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Content")));
	FString ScriptPath = FPaths::Combine(ContentDir, TEXT("folded_to_flamegraph.py"));

	if (!FPaths::FileExists(ScriptPath))
	{
		UE_LOG(LogNePython, Error, TEXT("Flamegraph script not found at path: %s"), *ScriptPath);
		return false;
	}

	// 2. Prepare and execute the Python script (use absolute paths)
	const FString ScriptPathAbs = FPaths::ConvertRelativePathToFull(ScriptPath);
	const FString FoldedFilePathAbs = FPaths::ConvertRelativePathToFull(FoldedFilePath);
	const FString OutputDirAbs = FPaths::ConvertRelativePathToFull(FPaths::GetPath(FoldedFilePathAbs));
	const FString Params = FString::Printf(TEXT("\"%s\" \"%s\" --output-dir \"%s\""),
		*ScriptPathAbs, *FoldedFilePathAbs, *OutputDirAbs);

	int32 ReturnCode = -1;
	FString StdOut;
	FString StdErr;

	UE_LOG(LogNePython, Log, TEXT("Executing flamegraph script: %s"), *Params);

	const FString PythonExePath = GetPythonExecutablePath();
	FPlatformProcess::ExecProcess(*PythonExePath, *Params, &ReturnCode, &StdOut, &StdErr, *ContentDir);

	if (ReturnCode != 0)
	{
		UE_LOG(LogNePython, Error, TEXT("Flamegraph script execution failed with code %d."), ReturnCode);
		UE_LOG(LogNePython, Error, TEXT("Script Stderr:\n%s"), *StdErr);
		UE_LOG(LogNePython, Error, TEXT("Script Stdout:\n%s"), *StdOut);
		return false;
	}

	// 3. Parse the output to find the SVG file path and open it
	FString SvgPath;
	TArray<FString> OutputLines;
	StdOut.ParseIntoArrayLines(OutputLines);

	const FString Prefix = TEXT("Flamegraph: ");
	for (const FString& Line : OutputLines)
	{
		if (Line.StartsWith(Prefix))
		{
			SvgPath = Line.RightChop(Prefix.Len());
			break;
		}
	}

	if (SvgPath.IsEmpty() || !FPaths::FileExists(SvgPath))
	{
		UE_LOG(LogNePython, Error, TEXT("Failed to find or parse SVG path from script output. Full output:\n%s"), *StdOut);
		return false;
	}

	UE_LOG(LogNePython, Log, TEXT("Successfully generated flamegraph: %s. Opening file..."), *SvgPath);
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*SvgPath);

	return true;
#else
	return FNePyFlameGraphExporter::ExportFoldedStacks(Allocations, StackTable, FilePath, Config);
#endif
}

bool FNePyFlameGraphExporter::ExportFoldedStacksDiff(
	const FString& FileBeforePath,
	const FString& FileThenPath,
	const FString& OutputFilePath)
{
#if WITH_EDITOR
	// 1. Locate the flamegraph diff script within the plugin
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("NePythonBinding"));
	if (!Plugin.IsValid())
	{
		UE_LOG(LogNePython, Error, TEXT("Could not find NePythonBinding plugin to locate flamegraph diff script."));
		return false;
	}
	const FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Content")));
	FString ScriptPath = FPaths::Combine(ContentDir, TEXT("memory_diff_folded.py"));

	if (!FPaths::FileExists(ScriptPath))
	{
		UE_LOG(LogNePython, Error, TEXT("Flamegraph diff script not found at path: %s"), *ScriptPath);
		return false;
	}

	// 2. Prepare and execute the Python script (use absolute paths)
	const FString ScriptPathAbs = FPaths::ConvertRelativePathToFull(ScriptPath);
	const FString FileBeforeAbs = FPaths::ConvertRelativePathToFull(FileBeforePath);
	const FString FileThenAbs = FPaths::ConvertRelativePathToFull(FileThenPath);
	const FString OutputFileAbs = FPaths::ConvertRelativePathToFull(OutputFilePath);

	const FString Params = FString::Printf(TEXT("\"%s\" \"%s\" \"%s\" -o \"%s\""),
		*ScriptPathAbs, *FileBeforeAbs, *FileThenAbs, *OutputFileAbs);

	int32 ReturnCode = -1;
	FString StdOut;
	FString StdErr;

	UE_LOG(LogNePython, Log, TEXT("Executing flamegraph diff script: %s"), *Params);

	const FString PythonExePath = GetPythonExecutablePath();
	FPlatformProcess::ExecProcess(*PythonExePath, *Params, &ReturnCode, &StdOut, &StdErr, *ContentDir);

	if (ReturnCode != 0)
	{
		UE_LOG(LogNePython, Error, TEXT("Flamegraph diff script execution failed with code %d."), ReturnCode);
		UE_LOG(LogNePython, Error, TEXT("Script Stderr:\n%s"), *StdErr);
		UE_LOG(LogNePython, Error, TEXT("Script Stdout:\n%s"), *StdOut);
		return false;
	}

	UE_LOG(LogNePython, Log, TEXT("Successfully generated flamegraph diff: %s."), *OutputFileAbs);
	return true;
#else
	UE_LOG(LogNePython, Error, TEXT("Flamegraph diff export is only available in the editor."));
	return false;
#endif
}

bool FNePyFlameGraphExporter::ExportSVGDiff(const FString& FileBefore, const FString& FileThen, const FString& FilePath)
{
#if WITH_EDITOR
	FString DiffFoldedPath = FPaths::ChangeExtension(FilePath, TEXT(".folded"));
	const FString DiffFoldedAbs = FPaths::ConvertRelativePathToFull(DiffFoldedPath);

	if (!ExportFoldedStacksDiff(FileBefore, FileThen, DiffFoldedAbs))
	{
		UE_LOG(LogNePython, Error, TEXT("Failed to generate intermediate diff folded file."));
		return false;
	}

	// 1. Locate the flamegraph generation script within the plugin
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("NePythonBinding"));
	if (!Plugin.IsValid())
	{
		UE_LOG(LogNePython, Error, TEXT("Could not find NePythonBinding plugin to locate flamegraph script."));
		return false;
	}
	const FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Content")));
	FString ScriptPath = FPaths::Combine(ContentDir, TEXT("folded_to_flamegraph.py"));

	if (!FPaths::FileExists(ScriptPath))
	{
		UE_LOG(LogNePython, Error, TEXT("Flamegraph script not found at path: %s"), *ScriptPath);
		return false;
	}

	// 2. Prepare and execute the Python script (use absolute paths)
	const FString ScriptPathAbs = FPaths::ConvertRelativePathToFull(ScriptPath);
	const FString OutputDirAbs = FPaths::ConvertRelativePathToFull(FPaths::GetPath(FilePath));

	const FString Params = FString::Printf(TEXT("\"%s\" \"%s\" --output-dir \"%s\""),
		*ScriptPathAbs, *DiffFoldedAbs, *OutputDirAbs);

	int32 ReturnCode = -1;
	FString StdOut;
	FString StdErr;

	UE_LOG(LogNePython, Log, TEXT("Executing flamegraph script for diff: %s"), *Params);

	const FString PythonExePath = GetPythonExecutablePath();
	FPlatformProcess::ExecProcess(*PythonExePath, *Params, &ReturnCode, &StdOut, &StdErr, *ContentDir);

	if (ReturnCode != 0)
	{
		UE_LOG(LogNePython, Error, TEXT("Flamegraph script execution failed with code %d."), ReturnCode);
		UE_LOG(LogNePython, Error, TEXT("Script Stderr:\n%s"), *StdErr);
		UE_LOG(LogNePython, Error, TEXT("Script Stdout:\n%s"), *StdOut);
		return false;
	}

	// 3. Parse the output to find the SVG file path and open it
	FString SvgPath;
	TArray<FString> OutputLines;
	StdOut.ParseIntoArrayLines(OutputLines);

	const FString Prefix = TEXT("Flamegraph: ");
	for (const FString& Line : OutputLines)
	{
		if (Line.StartsWith(Prefix))
		{
			SvgPath = Line.RightChop(Prefix.Len());
			break;
		}
	}

	if (SvgPath.IsEmpty() || !FPaths::FileExists(SvgPath))
	{
		UE_LOG(LogNePython, Error, TEXT("Failed to find or parse SVG path from script output. Full output:\n%s"), *StdOut);
		return false;
	}

	UE_LOG(LogNePython, Log, TEXT("Successfully generated flamegraph diff: %s. Opening file..."), *SvgPath);
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*SvgPath);

	return true;
#else
	UE_LOG(LogNePython, Error, TEXT("Flamegraph diff export is only available in the editor."));
	return false;
#endif
}

// ===== FNePyMemoryAllocator Implementation =====

FNePyMemoryAllocator* FNePyMemoryAllocator::Instance;
thread_local bool FNePyMemoryAllocator::bInAllocatorCall;

FNePyMemoryAllocator::FNePyMemoryAllocator()
	: bIsActive(false)
	, StartTime(0.0)
	, LastOptimizationTime(0.0)
{
	StartTime = FPlatformTime::Seconds();
	AllocatorFunctions.Malloc = FNePyMemoryAllocator::DefaultMalloc;
	AllocatorFunctions.Calloc = FNePyMemoryAllocator::DefaultCalloc;
	AllocatorFunctions.Realloc = FNePyMemoryAllocator::DefaultRealloc;
	AllocatorFunctions.Free = FNePyMemoryAllocator::DefaultFree;
}

FNePyMemoryAllocator::~FNePyMemoryAllocator()
{
	Shutdown();
}

FNePyMemoryAllocator& FNePyMemoryAllocator::Get()
{
	if (!Instance)
	{
		Instance = new FNePyMemoryAllocator();
	}
	return *Instance;
}

void FNePyMemoryAllocator::SetCustomAllocator(const FNePyCustomAllocator& CustomAllocator)
{
	AllocatorFunctions = CustomAllocator;
}

bool FNePyMemoryAllocator::Initialize()
{
	if (bIsActive) return true;

	UE_LOG(LogNePython, Log, TEXT("Initializing NePython Memory Allocator"));

#if NEPY_HAS_ALLOCATOR_API
	// Python 3.4+ allocator API
	PyMemAllocatorEx NewRawAllocator;
	NewRawAllocator.ctx = this;
	NewRawAllocator.malloc = PyRawMalloc;
	NewRawAllocator.calloc = PyRawCalloc;
	NewRawAllocator.realloc = PyRawRealloc;
	NewRawAllocator.free = PyRawFree;

	PyMemAllocatorEx NewMemAllocator;
	NewMemAllocator.ctx = this;
	NewMemAllocator.malloc = PyMemMalloc;
	NewMemAllocator.calloc = PyMemCalloc;
	NewMemAllocator.realloc = PyMemRealloc;
	NewMemAllocator.free = PyMemFree;

	PyMemAllocatorEx NewObjAllocator;
	NewObjAllocator.ctx = this;
	NewObjAllocator.malloc = PyObjMalloc;
	NewObjAllocator.calloc = PyObjCalloc;
	NewObjAllocator.realloc = PyObjRealloc;
	NewObjAllocator.free = PyObjFree;

	PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &OriginalRawAllocator);
	PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &OriginalMemAllocator);
	PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &OriginalObjAllocator);

	PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &NewRawAllocator);
	PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &NewMemAllocator);
	PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &NewObjAllocator);
#else
	// Python 2.x hook approach (limited functionality)
	OriginalMalloc = PyObject_Malloc;
	OriginalRealloc = PyObject_Realloc;
	OriginalFree = PyObject_Free;
	UE_LOG(LogNePython, Warning, TEXT("Using limited Python 2.x compatibility mode"));
#endif

	bIsActive = true;
	Stats.Reset();

	FNePyMemoryCommands::RegisterCommands();

	UE_LOG(LogNePython, Log, TEXT("NePy Memory Allocator initialized successfully"));
	return true;
}

void FNePyMemoryAllocator::Shutdown()
{
	if (!bIsActive) return;

	UE_LOG(LogNePython, Log, TEXT("Shutting down NePy Memory Allocator"));

	FNePyCallStack::CleanupThreadLocalCache();
	FNePyMemoryCommands::UnregisterCommands();

#if NEPY_HAS_ALLOCATOR_API
	// Restore original allocators
	PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &OriginalRawAllocator);
	PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &OriginalMemAllocator);
	PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &OriginalObjAllocator);
#endif

	bIsActive = false;

	UE_LOG(LogNePython, Log, TEXT("Final stats: %s"), *Stats.GetSummary());
	UE_LOG(LogNePython, Log, TEXT("NePy Memory Allocator shutdown complete"));
}

#if NEPY_HAS_ALLOCATOR_API
void* FNePyMemoryAllocator::PyRawMalloc(void* Ctx, size_t Size)
{
	/*
		PyMem_RawMalloc(0) means malloc(1). Some systems would return NULL
		for malloc(0), which would be treated as an error. Some platforms would
		return a pointer with no memory behind it, which would break pymalloc.
		To solve these problems, allocate an extra byte.
	*/
	if (Size == 0)
	{
		Size = 1;
	}

	auto Malloc = [](FNePyMemoryAllocator* Allocator, size_t Size) -> void*
	{
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_RAW_HOOK_DISABLE
		return Allocator->OriginalRawAllocator.malloc(Allocator->OriginalRawAllocator.ctx, Size);
#else
		return Allocator->AllocatorFunctions.Malloc(Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Malloc(Allocator, Size);
	}

	bInAllocatorCall = true;
	void* Ptr = Malloc(Allocator, Size);
	if (Ptr)
	{
		Allocator->TrackAllocation(Ptr, (uint32)Size, EPyMemoryDomain::Raw);
	}
	bInAllocatorCall = false;

	return Ptr;
}

void* FNePyMemoryAllocator::PyRawCalloc(void* Ctx, size_t Num, size_t Size)
{
	/*
		PyMem_RawCalloc(0, 0) means calloc(1, 1). Some systems would return NULL
		for calloc(0, 0), which would be treated as an error. Some platforms
		would return a pointer with no memory behind it, which would break
		pymalloc.  To solve these problems, allocate an extra byte.
	*/
	if (Num == 0 || Size == 0)
	{
		Num = 1;
		Size = 1;
	}

	auto Calloc = [](FNePyMemoryAllocator* Allocator, size_t Num, size_t Size) -> void*
	{
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_RAW_HOOK_DISABLE
		return Allocator->OriginalRawAllocator.calloc(Allocator->OriginalRawAllocator.ctx, Num, Size);
#else
		return Allocator->AllocatorFunctions.Calloc(Num, Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Calloc(Allocator, Num, Size);
	}

	bInAllocatorCall = true;
	void* Ptr = Calloc(Allocator, Num, Size);
	if (Ptr)
	{
		Allocator->TrackAllocation(Ptr, (uint32)(Num * Size), EPyMemoryDomain::Raw);
	}
	bInAllocatorCall = false;

	return Ptr;
}

void* FNePyMemoryAllocator::PyRawRealloc(void* Ctx, void* Ptr, size_t Size)
{
	// Handle zero size allocation edge case
	if (Size == 0)
	{
		Size = 1;
	}

	auto Realloc = [](FNePyMemoryAllocator* Allocator, void* Ptr, size_t Size) -> void*
	{
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_RAW_HOOK_DISABLE
		return Allocator->OriginalRawAllocator.realloc(Allocator->OriginalRawAllocator.ctx, Ptr, Size);
#else
		return Allocator->AllocatorFunctions.Realloc(Ptr, Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Realloc(Allocator, Ptr, Size);
	}

	bInAllocatorCall = true;

	// Track deallocation of old pointer if it exists
	if (Ptr)
	{
		Allocator->TrackDeallocation(Ptr);
	}

	void* NewPtr = Realloc(Allocator, Ptr, Size);
	if (NewPtr && Size > 0)
	{
		Allocator->TrackAllocation(NewPtr, (uint32)Size, EPyMemoryDomain::Raw);
	}

	bInAllocatorCall = false;
	return NewPtr;
}

void FNePyMemoryAllocator::PyRawFree(void* Ctx, void* Ptr)
{
	auto Free = [](FNePyMemoryAllocator* Allocator, void* Ptr) -> void
	{
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_RAW_HOOK_DISABLE
		return Allocator->OriginalRawAllocator.free(Allocator->OriginalRawAllocator.ctx, Ptr);
#else
		Allocator->AllocatorFunctions.Free(Ptr);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall || !Ptr)
	{
		Free(Allocator, Ptr);
		return;
	}

	bInAllocatorCall = true;
	Allocator->TrackDeallocation(Ptr);
	Free(Allocator, Ptr);
	bInAllocatorCall = false;
}

void* FNePyMemoryAllocator::PyMemMalloc(void* Ctx, size_t Size)
{
	if (Size == 0)
	{
		Size = 1;
	}

	auto Malloc = [](FNePyMemoryAllocator* Allocator, size_t Size) -> void*
	{
		FNePyScopedGIL GIL;
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_MEM_HOOK_DISABLE
		return Allocator->OriginalMemAllocator.malloc(Allocator->OriginalMemAllocator.ctx, Size);
#else
		return Allocator->AllocatorFunctions.Malloc(Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Malloc(Allocator, Size);
	}

	bInAllocatorCall = true;
	void* Ptr = Malloc(Allocator, Size);
	if (Ptr)
	{
		Allocator->TrackAllocation(Ptr, (uint32)Size, EPyMemoryDomain::Mem);
	}
	bInAllocatorCall = false;

	return Ptr;
}

void* FNePyMemoryAllocator::PyMemCalloc(void* Ctx, size_t Num, size_t Size)
{
	if (Num == 0 || Size == 0)
	{
		Num = 1;
		Size = 1;
	}

	auto Calloc = [](FNePyMemoryAllocator* Allocator, size_t Num, size_t Size) -> void*
	{
		FNePyScopedGIL GIL;
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_MEM_HOOK_DISABLE
		return Allocator->OriginalMemAllocator.calloc(Allocator->OriginalMemAllocator.ctx, Num, Size);
#else
		return Allocator->AllocatorFunctions.Calloc(Num, Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Calloc(Allocator, Num, Size);
	}

	bInAllocatorCall = true;
	void* Ptr = Calloc(Allocator, Num, Size);
	if (Ptr)
	{
		Allocator->TrackAllocation(Ptr, (uint32)(Num * Size), EPyMemoryDomain::Mem);
	}
	bInAllocatorCall = false;

	return Ptr;
}

void* FNePyMemoryAllocator::PyMemRealloc(void* Ctx, void* Ptr, size_t Size)
{
	if (Size == 0)
	{
		Size = 1;
	}

	auto Realloc = [](FNePyMemoryAllocator* Allocator, void* Ptr, size_t Size) ->void*
	{
		FNePyScopedGIL GIL;
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_MEM_HOOK_DISABLE
		return Allocator->OriginalMemAllocator.realloc(Allocator->OriginalMemAllocator.ctx, Ptr, Size);
#else
		if (Ptr == nullptr)
		{
			return Allocator->AllocatorFunctions.Malloc(Size);
		}
		return Allocator->AllocatorFunctions.Realloc(Ptr, Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Realloc(Allocator, Ptr, Size);
	}

	if (Ptr)
	{
		Allocator->TrackDeallocation(Ptr);
	}
	bInAllocatorCall = true;
	void* NewPtr = Realloc(Allocator, Ptr, Size);
	if (NewPtr && Size > 0)
	{
		Allocator->TrackAllocation(NewPtr, (uint32)Size, EPyMemoryDomain::Mem);
	}
	bInAllocatorCall = false;
	return NewPtr;
}

void FNePyMemoryAllocator::PyMemFree(void* Ctx, void* Ptr)
{
	auto Free = [](FNePyMemoryAllocator* Allocator, void* Ptr) -> void
	{
		FNePyScopedGIL GIL;
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_MEM_HOOK_DISABLE
		return Allocator->OriginalMemAllocator.free(Allocator->OriginalMemAllocator.ctx, Ptr);
#else
		if (Ptr)
		{
			Allocator->AllocatorFunctions.Free(Ptr);
		}
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall || !Ptr)
	{
		Free(Allocator, Ptr);
		return;
	}

	bInAllocatorCall = true;
	Allocator->TrackDeallocation(Ptr);
	Free(Allocator, Ptr);
	bInAllocatorCall = false;
}

void* FNePyMemoryAllocator::PyObjMalloc(void* Ctx, size_t Size)
{
	if (Size == 0)
	{
		Size = 1;
	}

	auto Malloc = [](FNePyMemoryAllocator* Allocator, size_t Size) -> void*
	{
		FNePyScopedGIL GIL;
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_OBJ_HOOK_DISABLE
		return Allocator->OriginalObjAllocator.malloc(Allocator->OriginalObjAllocator.ctx, Size);
#else
		return Allocator->AllocatorFunctions.Malloc(Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Malloc(Allocator, Size);
	}

	bInAllocatorCall = true;
	void* Ptr = Malloc(Allocator, Size);
	if (Ptr)
	{
		Allocator->TrackAllocation(Ptr, (uint32)Size, EPyMemoryDomain::Obj);
	}
	bInAllocatorCall = false;

	return Ptr;
}

void* FNePyMemoryAllocator::PyObjCalloc(void* Ctx, size_t Num, size_t Size)
{
	if (Num == 0 || Size == 0)
	{
		Num = 1;
		Size = 1;
	}

	auto Calloc = [](FNePyMemoryAllocator* Allocator, size_t Num, size_t Size) -> void*
	{
		FNePyScopedGIL GIL;
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_OBJ_HOOK_DISABLE
		return Allocator->OriginalObjAllocator.calloc(Allocator->OriginalObjAllocator.ctx, Num, Size);
#else
		return Allocator->AllocatorFunctions.Calloc(Num, Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Calloc(Allocator, Num, Size);
	}

	bInAllocatorCall = true;
	void* Ptr = Calloc(Allocator, Num, Size);
	if (Ptr)
	{
		Allocator->TrackAllocation(Ptr, (uint32)(Num * Size), EPyMemoryDomain::Obj);
	}
	bInAllocatorCall = false;

	return Ptr;
}

void* FNePyMemoryAllocator::PyObjRealloc(void* Ctx, void* Ptr, size_t Size)
{
	if (Size == 0)
	{
		Size = 1;
	}

	auto Realloc = [](FNePyMemoryAllocator* Allocator, void* Ptr, size_t Size) ->void*
	{
		FNePyScopedGIL GIL;
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_OBJ_HOOK_DISABLE
		return Allocator->OriginalObjAllocator.realloc(Allocator->OriginalObjAllocator.ctx, Ptr, Size);
#else
		if (Ptr == nullptr)
		{
			return Allocator->AllocatorFunctions.Malloc(Size);
		}
		return Allocator->AllocatorFunctions.Realloc(Ptr, Size);
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall)
	{
		return Realloc(Allocator, Ptr, Size);
	}

	if (Ptr)
	{
		Allocator->TrackDeallocation(Ptr);
	}
	bInAllocatorCall = true;
	void* NewPtr = Realloc(Allocator, Ptr, Size);
	if (NewPtr && Size > 0)
	{
		Allocator->TrackAllocation(NewPtr, (uint32)Size, EPyMemoryDomain::Obj);
	}
	bInAllocatorCall = false;
	return NewPtr;
}

void FNePyMemoryAllocator::PyObjFree(void* Ctx, void* Ptr)
{
	auto Free = [](FNePyMemoryAllocator* Allocator, void* Ptr) -> void
	{
		FNePyScopedGIL GIL;
		check("Make the lambda debugable");
#if NEPY_MEMORY_ALLOCATION_OBJ_HOOK_DISABLE
		return Allocator->OriginalObjAllocator.free(Allocator->OriginalObjAllocator.ctx, Ptr);
#else
		if (Ptr)
		{
			Allocator->AllocatorFunctions.Free(Ptr);
		}
#endif
	};

	FNePyMemoryAllocator* Allocator = static_cast<FNePyMemoryAllocator*>(Ctx);
	if (bInAllocatorCall || !Ptr)
	{
		Free(Allocator, Ptr);
		return;
	}

	bInAllocatorCall = true;
	Allocator->TrackDeallocation(Ptr);
	Free(Allocator, Ptr);
	bInAllocatorCall = false;
}

#else
void* FNePyMemoryAllocator::HookMalloc(size_t Size)
{
	auto Malloc = [](FNePyMemoryAllocator* Allocator, size_t Size) -> void*
	{
		check("Make the lambda debugable");
		return Allocator->AllocatorFunctions.Malloc(Size);
	};

	FNePyMemoryAllocator& Allocator = Get();

	if (bInAllocatorCall)
	{
#if NEPY_MEMORY_ALLOCATION_RAW_HOOK_DISABLE
		return OriginalMalloc(Size);
#else
		return Malloc(&Allocator, Size);
#endif
	}

	bInAllocatorCall = true;
	void* Ptr = Malloc(&Allocator, Size);
	if (Ptr)
	{
		Allocator.TrackAllocation(Ptr, (uint32)Size, EPyMemoryDomain::Unknown);
	}
	bInAllocatorCall = false;

	return Ptr;
}

void* FNePyMemoryAllocator::HookRealloc(void* Ptr, size_t NewSize)
{
	auto Realloc = [](FNePyMemoryAllocator* Allocator, void* Ptr, size_t NewSize) -> void*
	{
		check("Make the lambda debugable");
		return Allocator->AllocatorFunctions.Realloc(Ptr, NewSize);
	};

	FNePyMemoryAllocator& Allocator = Get();

	if (bInAllocatorCall)
	{
#if NEPY_MEMORY_ALLOCATION_RAW_HOOK_DISABLE
		return OriginalRealloc(Ptr, NewSize);
#else
		return Realloc(&Allocator, Ptr, NewSize);
#endif
	}

	bInAllocatorCall = true;
	// Track deallocation of old pointer if it exists
	if (Ptr)
	{
		Allocator.TrackDeallocation(Ptr);
	}
	void* NewPtr = Realloc(&Allocator, Ptr, NewSize);
	if (NewPtr)
	{
		Allocator.TrackAllocation(NewPtr, (uint32)NewSize, EPyMemoryDomain::Unknown);
	}
	bInAllocatorCall = false;

	return NewPtr;
}

void FNePyMemoryAllocator::HookFree(void* Ptr)
{
	auto Free = [](FNePyMemoryAllocator* Allocator, void* Ptr) -> void
	{
		check("Make the lambda debugable");
		Allocator->AllocatorFunctions.Free(Ptr);
	};

	// Early exit for null pointer
	if (!Ptr) return;

	FNePyMemoryAllocator& Allocator = Get();

	if (bInAllocatorCall)
	{
#if NEPY_MEMORY_ALLOCATION_RAW_HOOK_DISABLE
		OriginalFree(Ptr);
#else
		Free(&Allocator, Ptr);
#endif
		return;
	}

	bInAllocatorCall = true;
	Allocator.TrackDeallocation(Ptr);
	Free(&Allocator, Ptr);
	bInAllocatorCall = false;
}
#endif

void* FNePyMemoryAllocator::DefaultMalloc(size_t Size)
{
	LLM_SCOPE_BYTAG(NePython);
	return FMemory::Malloc(Size);
}

void* FNePyMemoryAllocator::DefaultCalloc(size_t NumElements, size_t ElementSize)
{
	LLM_SCOPE_BYTAG(NePython);
	void* Ptr = FMemory::Malloc(NumElements * ElementSize);
	if (Ptr) FMemory::Memzero(Ptr, NumElements * ElementSize);
	return Ptr;
}

void* FNePyMemoryAllocator::DefaultRealloc(void* Ptr, size_t NewSize)
{
	LLM_SCOPE_BYTAG(NePython);
	return FMemory::Realloc(Ptr, NewSize, 8);
}

void FNePyMemoryAllocator::DefaultFree(void* Ptr)
{
	LLM_SCOPE_BYTAG(NePython);
	FMemory::Free(Ptr);
}

void FNePyMemoryAllocator::BindOwnerIfTracked(PyObject* Obj)
{
	if (!bIsActive || !GNePyMemoryTracking || Obj == nullptr)
	{
	 return;
	}

	// We only store the pointer as Owner; do not dereference fields here.
	FScopeLock Lock(&CriticalSection);

	// Allocations are keyed by the raw pointer. For OBJ domain, the key is the PyObject* itself.
	if (FNePyAllocationInfo* Info = CurrentAllocations.Find(static_cast<void*>(Obj)))
	{
		Info->Owner = Obj;
	}
}

void FNePyMemoryAllocator::TrackAllocation(void* Ptr, uint32 Size, EPyMemoryDomain Domain)
{
	if (!Py_IsInitialized() || !GNePyMemoryTracking)
	{
		return;
	}

	FScopeLock Lock(&CriticalSection);

	FNePyCallStack Stack;
	if (Stack.CaptureCurrentCallStack(GNePyMemoryMaxStack, false))
	{
		uint32 StackIndex = GetOrCreateStackIndex(Stack);

		FNePyAllocationInfo AllocInfo(Ptr, Size, nullptr);
		AllocInfo.StackIndex = StackIndex;

		CurrentAllocations.Add(Ptr, AllocInfo);
		Stats.UpdateAllocation(Size);
	}
}

void FNePyMemoryAllocator::TrackDeallocation(void* Ptr)
{
	if (!Py_IsInitialized() || !GNePyMemoryTracking)
	{
		return;
	}

	FScopeLock Lock(&CriticalSection);

	FNePyAllocationInfo* AllocInfo = CurrentAllocations.Find(Ptr);
	if (AllocInfo)
	{
		Stats.UpdateDeallocation(AllocInfo->Size);
		CurrentAllocations.Remove(Ptr);
	}
}

uint32 FNePyMemoryAllocator::GetOrCreateStackIndex(const FNePyCallStack& Stack)
{
	uint32 Hash = Stack.GetTypeHash();
	uint32* ExistingIndex = StackHashToIndex.Find(Hash);
	if (ExistingIndex) return *ExistingIndex;

	uint32 NewIndex = StackTable.Add(Stack);
	StackHashToIndex.Add(Hash, NewIndex);
	return NewIndex;
}

const FNePyCallStack* FNePyMemoryAllocator::GetStackByIndex(uint32 Index) const
{
	FScopeLock Lock(&CriticalSection);
	if (Index >= (uint32)StackTable.Num())
	{
		return nullptr;
	}
	return &StackTable[Index];
}

uint32 FNePyMemoryAllocator::GetCompactTime()
{
	static double StartTime = FPlatformTime::Seconds();
	double Elapsed = FPlatformTime::Seconds() - StartTime;
	return (uint32)(Elapsed * 10.0); // 100ms resolution
}

const FNePyMemoryStats& FNePyMemoryAllocator::GetMemoryStats() const
{
	FScopeLock Lock(&CriticalSection);
	return Stats;
}

const TArray<FNePyAllocationInfo>& FNePyMemoryAllocator::GetActiveAllocations() const
{
	FScopeLock Lock(&CriticalSection);

	static TArray<FNePyAllocationInfo> CachedAllocations;
	CachedAllocations.Reset();
	CachedAllocations.Reserve(CurrentAllocations.Num());

	for (const auto& Pair : CurrentAllocations)
	{
		CachedAllocations.Add(Pair.Value);
	}

	return CachedAllocations;
}

const TArray<FNePyCallStack>& FNePyMemoryAllocator::GetStackTable() const
{
	FScopeLock Lock(&CriticalSection);
	return StackTable;
}

bool FNePyMemoryAllocator::ExportFlameGraph(const FString& FilePath) const
{
	return ExportFlameGraphWithConfig(FilePath, FNePyFlameGraphConfig());
}

bool FNePyMemoryAllocator::ExportFlameGraphWithConfig(const FString& FilePath, const FNePyFlameGraphConfig& Config) const
{
	NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

	// Snapshot under lock, then process outside
	TArray<FNePyAllocationInfo> Allocations;
	TArray<FNePyCallStack>      StackTableSnapshot;
	{
		FScopeLock Lock(&CriticalSection);
		CurrentAllocations.GenerateValueArray(Allocations);
		StackTableSnapshot = StackTable;
	}

	return FNePyFlameGraphExporter::ExportSVG(Allocations, StackTableSnapshot, FilePath, Config);
}

bool FNePyMemoryAllocator::ExportFlameGraphFolded(const FString& FilePath) const
{
	return ExportFlameGraphFoldedWithConfig(FilePath, FNePyFlameGraphConfig());
}

bool FNePyMemoryAllocator::ExportFlameGraphFoldedWithConfig(const FString& FilePath, const FNePyFlameGraphConfig& Config) const
{
	NEPY_MEMORY_ALLOCATOR_TRACKING_DISABLE_SCOPE;

	// Snapshot under lock, then process outside
	TArray<FNePyAllocationInfo> Allocations;
	TArray<FNePyCallStack>      StackTableSnapshot;
	{
		FScopeLock Lock(&CriticalSection);
		CurrentAllocations.GenerateValueArray(Allocations);
		StackTableSnapshot = StackTable;
	}

	return FNePyFlameGraphExporter::ExportFoldedStacks(Allocations, StackTableSnapshot, FilePath, Config);
}

bool FNePyMemoryAllocator::ExportReport(const FString& FilePath) const
{
	FScopeLock Lock(&CriticalSection);

	if (!bIsActive)
	{
		return false;
	}

	FString ReportContent = GenerateDetailedReport();

	// Ensure directory exists
	FString Directory = FPaths::GetPath(FilePath);
	if (!Directory.IsEmpty())
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*Directory))
		{
			PlatformFile.CreateDirectoryTree(*Directory);
		}
	}

	// Write to file
	return FFileHelper::SaveStringToFile(ReportContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

FString FNePyMemoryAllocator::GenerateDetailedReport() const
{
	FString Report;

	// Header
	Report += TEXT("      NePy Memory Allocation Report\n");

	// Current timestamp
	FDateTime Now = FDateTime::Now();
	Report += FString::Printf(TEXT("Generated at: %s\n\n"), *Now.ToString());

	// Overall statistics
	Report += TEXT("OVERALL STATISTICS\n");
	Report += TEXT("------------------\n");
	Report += FString::Printf(TEXT("Current Allocated: %.2f MB (%llu bytes)\n"),
		Stats.GetCurrentAllocated() / (1024.0 * 1024.0), Stats.GetCurrentAllocated());
	Report += FString::Printf(TEXT("Peak Allocated: %.2f MB (%llu bytes)\n"),
		Stats.GetPeakAllocated() / (1024.0 * 1024.0), Stats.GetPeakAllocated());
	Report += FString::Printf(TEXT("Total Allocations: %llu\n"), Stats.GetTotalAllocations());
	Report += FString::Printf(TEXT("Active Allocations: %u\n\n"), Stats.GetActiveAllocations());

	// Memory statistics grouped by Python object type
	TMap<FString, uint64> AllocationsByType;
	TMap<FString, uint32> CountsByType;

	// Collect statistics by Python object type
	for (const auto& Pair : CurrentAllocations)
	{
		const FNePyAllocationInfo& AllocInfo = Pair.Value;
		FNePyObjectInfo ObjectInfo(AllocInfo.Owner);
		const FString& TypeName = ObjectInfo.TypeName;

		// Use "Unknown" if TypeName is empty
		FString ActualTypeName = TypeName.IsEmpty() ? TEXT("Unknown") : TypeName;

		AllocationsByType.FindOrAdd(ActualTypeName) += AllocInfo.Size;
		CountsByType.FindOrAdd(ActualTypeName)++;
	}

	if (AllocationsByType.Num() > 0)
	{
		Report += TEXT("ALLOCATIONS BY PYTHON TYPE\n");
		Report += TEXT("----------------------------\n");

		TArray<TPair<FString, uint64>> SortedBySize;
		for (const auto& TypePair : AllocationsByType)
		{
			SortedBySize.Add(TypePair);
		}
		SortedBySize.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });

		Report += FString::Printf(TEXT("%-25s %12s %8s %6s %s\n"),
			TEXT("Python Type"), TEXT("Size"), TEXT("Count"), TEXT("%"), TEXT("Flags"));
		Report += TEXT("--------------------------------------------------------------------------------\n");

		for (const auto& TypePair : SortedBySize)
		{
			uint32 Count = CountsByType[TypePair.Key];
			double Percentage = Stats.GetCurrentAllocated() > 0 ?
				(double)TypePair.Value / Stats.GetCurrentAllocated() * 100.0 : 0.0;

			FString SizeStr;
			if (TypePair.Value >= 1024 * 1024)
			{
				SizeStr = FString::Printf(TEXT("%.1f MB"), TypePair.Value / (1024.0 * 1024.0));
			}
			else if (TypePair.Value >= 1024)
			{
				SizeStr = FString::Printf(TEXT("%.1f KB"), TypePair.Value / 1024.0);
			}
			else
			{
				SizeStr = FString::Printf(TEXT("%llu B"), TypePair.Value);
			}

			// Get common flags for this type (from first allocation)
			FString FlagsStr = TEXT("-");
			for (const auto& AllocPair : CurrentAllocations)
			{
				FNePyObjectInfo ObjectInfo(AllocPair.Value.Owner);
				if (ObjectInfo.TypeName == TypePair.Key)
				{
					FlagsStr = GetObjectFlagsString(ObjectInfo.ObjectFlags);
					break;
				}
			}

			Report += FString::Printf(TEXT("%-25s %12s %8u %5.1f%% %s\n"),
				*TypePair.Key, *SizeStr, Count, Percentage, *FlagsStr);
		}
		Report += TEXT("\n");
	}

	// Group allocations by call stack
	TMap<uint32, TArray<const FNePyAllocationInfo*>> AllocsByStack;

	for (const auto& Pair : CurrentAllocations)
	{
		const FNePyAllocationInfo& AllocInfo = Pair.Value;
		AllocsByStack.FindOrAdd(AllocInfo.StackIndex).Add(&AllocInfo);
	}

	// Calculate statistics for each call stack
	struct FNePyTopAllocator
	{
		uint32 StackIndex;
		uint32 Count;
		uint64 TotalSize;
		FString Description;
	};

	TArray<FNePyTopAllocator> TopAllocators;
	for (const auto& StackPair : AllocsByStack)
	{
		FNePyTopAllocator Top;
		Top.StackIndex = StackPair.Key;
		Top.Count = StackPair.Value.Num();
		Top.TotalSize = 0;

		// Sum up total size for this call stack
		for (const FNePyAllocationInfo* Alloc : StackPair.Value)
		{
			Top.TotalSize += Alloc->Size;
		}

		// Get call stack description
		if (Top.StackIndex < (uint32)StackTable.Num())
		{
			Top.Description = StackTable[Top.StackIndex].GetTopFunction();
		}
		else
		{
			Top.Description = TEXT("Unknown");
		}

		TopAllocators.Add(Top);
	}

	// Sort by total size
	TopAllocators.Sort([](const FNePyTopAllocator& A, const FNePyTopAllocator& B) {
		return A.TotalSize > B.TotalSize;
		});

	// Top allocators report
	if (TopAllocators.Num() > 0)
	{
		Report += TEXT("TOP ALLOCATION CALL STACKS\n");
		Report += TEXT("---------------------------\n");
		Report += FString::Printf(TEXT("%-8s %-12s %-8s %-15s %s\n"),
			TEXT("Rank"), TEXT("Size"), TEXT("Count"), TEXT("Avg Size"), TEXT("Top Function"));
		Report += TEXT("--------------------------------------------------------------------------------\n");

		int32 TopCount = FMath::Min(10, TopAllocators.Num());
		for (int32 i = 0; i < TopCount; ++i)
		{
			const auto& Top = TopAllocators[i];

			FString SizeStr;
			if (Top.TotalSize >= 1024 * 1024)
			{
				SizeStr = FString::Printf(TEXT("%.1f MB"), Top.TotalSize / (1024.0 * 1024.0));
			}
			else if (Top.TotalSize >= 1024)
			{
				SizeStr = FString::Printf(TEXT("%.1f KB"), Top.TotalSize / 1024.0);
			}
			else
			{
				SizeStr = FString::Printf(TEXT("%llu B"), Top.TotalSize);
			}

			uint32 AvgSize = Top.Count > 0 ? (uint32)(Top.TotalSize / Top.Count) : 0;
			FString AvgSizeStr = FString::Printf(TEXT("%u B"), AvgSize);

			Report += FString::Printf(TEXT("#%-7d %-12s %-8u %-15s %s\n"),
				i + 1, *SizeStr, Top.Count, *AvgSizeStr, *Top.Description);

			// Show sample allocations for this call stack
			const auto& AllocsForStack = AllocsByStack[Top.StackIndex];
			Report += TEXT("Sample Allocations:\n");

			int32 SampleCount = FMath::Min(5, AllocsForStack.Num());
			for (int32 j = 0; j < SampleCount; ++j)
			{
				const auto* Alloc = AllocsForStack[j];
				FNePyObjectInfo ObjectInfo(Alloc->Owner);
				double Age = Alloc->GetAllocTimeSeconds();
				FString TypeName = ObjectInfo.TypeName.IsEmpty() ?
					TEXT("Unknown") : ObjectInfo.TypeName;

				FString UObjectStr;
				if (!ObjectInfo.UObjectName.IsEmpty())
				{
					UObjectStr = FString::Printf(TEXT(", uobject: %s"), *ObjectInfo.UObjectName);
				}

				Report += FString::Printf(TEXT("  - %u bytes, age: %.1f sec, type: %s, thread: %u%s\n"),
					Alloc->Size, Age, *TypeName, Alloc->ThreadId, *UObjectStr);
			}

			if (i < TopCount - 1)
			{
				Report += TEXT("\n");
			}
		}
		Report += TEXT("\n");
	}

	// Potential memory leak detection
	double LeakThreshold = 300.0; // 5 minutes
	TArray<const FNePyAllocationInfo*> PotentialLeaks;

	for (const auto& AllocPair : CurrentAllocations)
	{
		const FNePyAllocationInfo& AllocInfo = AllocPair.Value;
		double Age = AllocInfo.GetAllocTimeSeconds();

		if (Age > LeakThreshold)
		{
			PotentialLeaks.Add(&AllocInfo);
		}
	}

	if (PotentialLeaks.Num() > 0)
	{
		Report += TEXT("POTENTIAL MEMORY LEAKS\n");
		Report += TEXT("----------------------\n");
		Report += FString::Printf(TEXT("Allocations older than %.0f seconds (%d found):\n\n"),
			LeakThreshold, PotentialLeaks.Num());

		// Sort by age (oldest first)
		PotentialLeaks.Sort([](const FNePyAllocationInfo& A, const FNePyAllocationInfo& B) {
			return A.GetAllocTimeSeconds() > B.GetAllocTimeSeconds();
			});

		Report += FString::Printf(TEXT("%-8s %-12s %-25s %-4s %s\n"),
			TEXT("Age"), TEXT("Size"), TEXT("Type / UObject"), TEXT("T#"), TEXT("Function"));
		Report += TEXT("--------------------------------------------------------------------------------\n");

		int32 LeakCount = FMath::Min(20, PotentialLeaks.Num());
		for (int32 i = 0; i < LeakCount; ++i)
		{
			const auto* Leak = PotentialLeaks[i];
			FNePyObjectInfo ObjectInfo(Leak->Owner);
			double Age = Leak->GetAllocTimeSeconds();

			FString SizeStr = FString::Printf(TEXT("%.1f KB"), Leak->Size / 1024.0);
			FString TypeStr = ObjectInfo.TypeName.IsEmpty() ?
				TEXT("Unknown") : ObjectInfo.TypeName;

			if (!ObjectInfo.UObjectName.IsEmpty())
			{
				TypeStr = FString::Printf(TEXT("%s (%s)"), *TypeStr, *ObjectInfo.UObjectName);
			}

			FString FunctionStr = TEXT("Unknown");
			if (Leak->StackIndex < (uint32)StackTable.Num())
			{
				FunctionStr = StackTable[Leak->StackIndex].GetTopFunction();
			}

			Report += FString::Printf(TEXT("%-8.0fs %-12s %-25s T%u %s\n"),
				Age, *SizeStr, *TypeStr, Leak->ThreadId, *FunctionStr);
		}

		if (PotentialLeaks.Num() > LeakCount)
		{
			Report += FString::Printf(TEXT("... and %d more\n"), PotentialLeaks.Num() - LeakCount);
		}
		Report += TEXT("\n");
	}

	// Footer
	Report += TEXT("           End of Report\n");

	return Report;
}

// Helper function to convert object flags to string
FString FNePyMemoryAllocator::GetObjectFlagsString(uint8 Flags) const
{
	TArray<FString> FlagNames;

	if (Flags & FNePyObjectInfo::FLAG_IS_UOBJECT) FlagNames.Add(TEXT("U"));
	if (Flags & FNePyObjectInfo::FLAG_IS_CONTAINER) FlagNames.Add(TEXT("C"));
	if (Flags & FNePyObjectInfo::FLAG_IS_STRING) FlagNames.Add(TEXT("S"));
	if (Flags & FNePyObjectInfo::FLAG_IS_NUMERIC) FlagNames.Add(TEXT("N"));
	if (Flags & FNePyObjectInfo::FLAG_IS_BUILTIN) FlagNames.Add(TEXT("B"));
	if (Flags & FNePyObjectInfo::FLAG_IS_CALLABLE) FlagNames.Add(TEXT("F"));

	if (FlagNames.Num() == 0)
	{
		return TEXT("-");
	}

	return FString::Join(FlagNames, TEXT(","));
}

void FNePyMemoryAllocator::ClearTrackingData()
{
	FScopeLock Lock(&CriticalSection);
	CurrentAllocations.Reset();
	StackTable.Reset();
	StackHashToIndex.Reset();
	Stats.Reset();
}