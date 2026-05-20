#pragma once
#include "NePyBase.h"
#include "CoreMinimal.h"

/**
 * 可追踪的 Python 对象基类
 * 提供对象分配位置追踪、生命周期监控等调试功能
 */
class NEPYTHONBINDING_API FNePyTrackedObject
{
public:
#if NEPY_ENABLE_PYTHON_OBJECT_TRACKING
	// === 追踪信息字段 ===
	/** Python 文件路径 */
	FString TrackedFileName;

	/** 行号 */
	int32 TrackedLineNumber;

	/** 函数名 */
	FString TrackedFunctionName;

	/** 行内容 */
	FString TrackedLineContent;

	/** 分配时间戳 */
	FDateTime TrackedAllocationTime;

	/** 线程 ID */
	uint32 TrackedThreadId;

	/** 追踪信息是否有效 */
	bool bTrackingInfoValid;

	/** 对象类型名称 */
	FString TrackedObjectTypeName;

	/** 对象唯一 ID */
	uint64 TrackedObjectId;

	/** 全局对象 ID 计数器 */
	static TAtomic<uint64> GlobalObjectIdCounter;
#endif

public:
	// === 构造函数 ===
	FNePyTrackedObject()
#if NEPY_ENABLE_PYTHON_OBJECT_TRACKING
		: TrackedLineNumber(-1)
		, TrackedAllocationTime(FDateTime::Now())
		, TrackedThreadId(FPlatformTLS::GetCurrentThreadId())
		, bTrackingInfoValid(false)
		, TrackedObjectId(++GlobalObjectIdCounter)
#endif
	{
	}

	explicit FNePyTrackedObject(const FString& ObjectTypeName)
#if NEPY_ENABLE_PYTHON_OBJECT_TRACKING
		: TrackedLineNumber(-1)
		, TrackedAllocationTime(FDateTime::Now())
		, TrackedThreadId(FPlatformTLS::GetCurrentThreadId())
		, bTrackingInfoValid(false)
		, TrackedObjectTypeName(ObjectTypeName)
		, TrackedObjectId(++GlobalObjectIdCounter)
#endif
	{
	}

	~FNePyTrackedObject() = default;

#if NEPY_ENABLE_PYTHON_OBJECT_TRACKING
	// === 追踪相关方法 ===

	/** 捕获分配位置信息 */
	void CaptureAllocationInfo(int32 SkipFrames = 0);

	/** 从 Python 调用栈捕获信息（用于在 tp_new 中调用）*/
	void CaptureAllocationInfoFromPython(int32 SkipFrames = 0);

	/** 手动设置追踪信息 */
	void SetTrackingInfo(const FString& FileName, int32 LineNumber, const FString& FunctionName, const FString& LineContent = TEXT(""));

	/** 设置对象类型名称 */
	void SetObjectTypeName(const FString& TypeName) { TrackedObjectTypeName = TypeName; }

	/** 获取完整的追踪信息字符串 */
	FString GetTrackingInfoString() const;

	/** 获取简短的追踪信息（仅文件名和行号）*/
	FString GetShortTrackingInfo() const;

	/** 获取对象类型名称 */
	FString GetObjectTypeName() const { return TrackedObjectTypeName; }

	/** 获取对象唯一 ID */
	uint64 GetObjectId() const { return TrackedObjectId; }

	/** 获取行内容 */
	FString GetLineContent() const { return TrackedLineContent; }

	/** 检查追踪信息是否有效 */
	bool HasValidTrackingInfo() const { return bTrackingInfoValid; }

	/** 清除追踪信息 */
	void ClearTrackingInfo();

	/** 获取分配时间戳 */
	FDateTime GetAllocationTime() const { return TrackedAllocationTime; }

	/** 获取分配线程ID */
	uint32 GetAllocationThreadId() const { return TrackedThreadId; }

	/** 获取对象生命周期（秒）*/
	double GetObjectLifetimeSeconds() const;

	/** 格式化文件路径用于显示 */
	FString FormatFilePathForDisplay(const FString& FilePath) const;

	// === Python 接口方法 ===
	static PyObject* PyGetTrackingInfo(PyObject* Self);
	static PyObject* PyGetAllocationTime(PyObject* Self);
	static PyObject* PyHasTrackingInfo(PyObject* Self);
	static PyObject* PyGetObjectTypeName(PyObject* Self);
	static PyObject* PyGetObjectId(PyObject* Self);
	static PyObject* PyGetObjectLifetime(PyObject* Self);
	static PyObject* PyGetLineContent(PyObject* Self);

protected:
	/** 从 C++ 调用栈捕获信息 */
	bool CaptureFromCppStack(int32 SkipFrames);

	/** 从 Python 调用栈捕获信息 */
	bool CaptureFromPythonStack(int32 SkipFrames);

	/** 判断是否应该跳过某个文件名 */
	bool ShouldSkipFileName(const FString& FileName) const;

	/** 清理 Python 错误状态 */
	void ClearPythonErrors();
#else
	// === 空实现（追踪禁用时） ===
	void CaptureAllocationInfo(int32 SkipFrames = 0) {}
	void CaptureAllocationInfoFromPython(int32 SkipFrames = 0) {}
	void SetTrackingInfo(const FString& FileName, int32 LineNumber, const FString& FunctionName, const FString& LineContent = TEXT("")) {}
	void SetObjectTypeName(const FString& TypeName) {}
	FString GetTrackingInfoString() const { return TEXT("Object tracking disabled"); }
	FString GetShortTrackingInfo() const { return TEXT("N/A"); }
	FString GetObjectTypeName() const { return TEXT("Unknown"); }
	uint64 GetObjectId() const { return 0; }
	FString GetLineContent() const { return TEXT("N/A"); }
	bool HasValidTrackingInfo() const { return false; }
	void ClearTrackingInfo() {}
	FDateTime GetAllocationTime() const { return FDateTime::MinValue(); }
	uint32 GetAllocationThreadId() const { return 0; }
	double GetObjectLifetimeSeconds() const { return 0.0; }
	FString FormatFilePathForDisplay(const FString& FilePath) const { return TEXT("N/A"); }

	static PyObject* PyGetTrackingInfo(PyObject* Self) { Py_RETURN_NONE; }
	static PyObject* PyGetAllocationTime(PyObject* Self) { Py_RETURN_NONE; }
	static PyObject* PyHasTrackingInfo(PyObject* Self) { Py_RETURN_FALSE; }
	static PyObject* PyGetObjectTypeName(PyObject* Self) { Py_RETURN_NONE; }
	static PyObject* PyGetObjectId(PyObject* Self) { return PyLong_FromLong(0); }
	static PyObject* PyGetObjectLifetime(PyObject* Self) { return PyFloat_FromDouble(0.0); }
	static PyObject* PyGetLineContent(PyObject* Self) { Py_RETURN_NONE; }
#endif
};

// === 便捷宏定义 ===
#if NEPY_ENABLE_PYTHON_OBJECT_TRACKING
	/** 在 tp_new 中调用此宏记录对象创建信息 */
#define NEPY_RECORD_OBJECT_CREATION(TrackedObj, TypeName) \
		do { \
			new (TrackedObj) FNePyTrackedObject();\
			(TrackedObj)->SetObjectTypeName(TypeName); \
			(TrackedObj)->CaptureAllocationInfoFromPython(); \
		} while(0)
#else
#define NEPY_RECORD_OBJECT_CREATION(TrackedObj, TypeName)
#endif

// === Python 方法表宏定义 ===
#if NEPY_ENABLE_PYTHON_OBJECT_TRACKING
#define NEPY_TRACKED_OBJECT_METHODS \
	{"get_tracking_info", (PyCFunction)FNePyTrackedObject::PyGetTrackingInfo, METH_NOARGS, \
	 "Get object allocation tracking information"}, \
	{"get_allocation_time", (PyCFunction)FNePyTrackedObject::PyGetAllocationTime, METH_NOARGS, \
	 "Get allocation timestamp"}, \
	{"has_tracking_info", (PyCFunction)FNePyTrackedObject::PyHasTrackingInfo, METH_NOARGS, \
	 "Check if tracking information is available"}, \
	{"get_object_type_name", (PyCFunction)FNePyTrackedObject::PyGetObjectTypeName, METH_NOARGS, \
	 "Get object type name"}, \
	{"get_object_id", (PyCFunction)FNePyTrackedObject::PyGetObjectId, METH_NOARGS, \
	 "Get unique object ID"}, \
	{"get_object_lifetime", (PyCFunction)FNePyTrackedObject::PyGetObjectLifetime, METH_NOARGS, \
	 "Get object lifetime in seconds"}, \
	{"get_line_content", (PyCFunction)FNePyTrackedObject::PyGetLineContent, METH_NOARGS, \
	 "Get source code line content"},
#else
#define NEPY_TRACKED_OBJECT_METHODS
#endif