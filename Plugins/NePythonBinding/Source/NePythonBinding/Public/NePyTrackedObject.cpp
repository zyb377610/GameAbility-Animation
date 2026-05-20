#include "NePyTrackedObject.h"
#include "Misc/Paths.h"

#if NEPY_ENABLE_PYTHON_OBJECT_TRACKING

// 静态成员定义
TAtomic<uint64> FNePyTrackedObject::GlobalObjectIdCounter(0);

void FNePyTrackedObject::CaptureAllocationInfo(int32 SkipFrames)
{
	// 优先尝试从 Python 调用栈获取信息
	if (Py_IsInitialized() && CaptureFromPythonStack(SkipFrames))
	{
		bTrackingInfoValid = true;
		return;
	}

	// 如果 Python 调用栈失败，尝试从 C++ 调用栈获取
	if (CaptureFromCppStack(SkipFrames))
	{
		bTrackingInfoValid = true;
	}
}

void FNePyTrackedObject::CaptureAllocationInfoFromPython(int32 SkipFrames)
{
	TrackedAllocationTime = FDateTime::Now();
	TrackedThreadId = FPlatformTLS::GetCurrentThreadId();

	if (CaptureFromPythonStack(SkipFrames))
	{
		bTrackingInfoValid = true;
	}
}

bool FNePyTrackedObject::CaptureFromPythonStack(int32 SkipFrames)
{
	if (!Py_IsInitialized())
	{
		return false;
	}

	if (PyErr_Occurred())
	{
		// UE_LOG(LogNePython, Warning, TEXT("Error in Python interpreter before capturing stack."));
		return false;
	}

	PyObject* TracebackModule = PyImport_ImportModule("traceback");
	if (!TracebackModule)
	{
		ClearPythonErrors();
		return false;
	}

	PyObject* ExtractStack = PyObject_GetAttrString(TracebackModule, "extract_stack");
	if (!ExtractStack || !PyCallable_Check(ExtractStack))
	{
		Py_DECREF(TracebackModule);
		ClearPythonErrors();
		return false;
	}

	PyObject* Stack = PyObject_CallFunction(ExtractStack, NULL);
	bool bSuccess = false;

	if (Stack && PyList_Check(Stack))
	{
		Py_ssize_t StackSize = PyList_Size(Stack);

		for (Py_ssize_t i = StackSize - SkipFrames - 1; i >= 0; --i)
		{
			PyObject* FrameInfo = PyList_GetItem(Stack, i);

			if (!FrameInfo)
			{
				continue;
			}

			PyObject* FileNameObj = nullptr;
			PyObject* LineNoObj = nullptr;
			PyObject* FuncNameObj = nullptr;
			PyObject* LineContentObj = nullptr;

			if (strcmp(Py_TYPE(FrameInfo)->tp_name, "FrameSummary") == 0)
			{
				FileNameObj = PyObject_GetAttrString(FrameInfo, "filename");
				LineNoObj = PyObject_GetAttrString(FrameInfo, "lineno");
				FuncNameObj = PyObject_GetAttrString(FrameInfo, "name");
				LineContentObj = PyObject_GetAttrString(FrameInfo, "line");
			}
			else if (PyTuple_Check(FrameInfo))
			{
				if (PyTuple_Size(FrameInfo) < 3)
				{
					continue;
				}
				FileNameObj = PyTuple_GetItem(FrameInfo, 0);
				LineNoObj = PyTuple_GetItem(FrameInfo, 1);
				FuncNameObj = PyTuple_GetItem(FrameInfo, 2);
				// 对于 tuple 格式，line 内容通常在索引3（如果存在）
				if (PyTuple_Size(FrameInfo) > 3)
				{
					LineContentObj = PyTuple_GetItem(FrameInfo, 3);
				}
			}

			if (FileNameObj && LineNoObj && FuncNameObj)
			{
				const char* FileNameStr = nullptr;
				const char* FuncNameStr = nullptr;
				const char* LineContentStr = nullptr;
				long LineNum = -1;

#if PY_MAJOR_VERSION >= 3
				FileNameStr = PyUnicode_AsUTF8(FileNameObj);
				FuncNameStr = PyUnicode_AsUTF8(FuncNameObj);
				LineNum = PyLong_AsLong(LineNoObj);

				// 处理行内容：可能是字符串或 None
				if (LineContentObj && LineContentObj != Py_None && PyUnicode_Check(LineContentObj))
				{
					LineContentStr = PyUnicode_AsUTF8(LineContentObj);
				}
#else
				if (PyString_Check(FileNameObj))
				{
					FileNameStr = PyString_AsString(FileNameObj);
				}
				if (PyString_Check(FuncNameObj))
				{
					FuncNameStr = PyString_AsString(FuncNameObj);
				}
				LineNum = PyInt_AsLong(LineNoObj);

				// 处理行内容
				if (LineContentObj && LineContentObj != Py_None && PyString_Check(LineContentObj))
				{
					LineContentStr = PyString_AsString(LineContentObj);
				}
#endif

				if (FileNameStr && FuncNameStr)
				{
					FString TempFileName(ANSI_TO_TCHAR(FileNameStr));

					if (!ShouldSkipFileName(TempFileName))
					{
						TrackedFileName = TempFileName;
						TrackedLineNumber = (int32)LineNum;
						TrackedFunctionName = FString(ANSI_TO_TCHAR(FuncNameStr));

						// 设置行内容
						if (LineContentStr)
						{
							FString RawLineContent(ANSI_TO_TCHAR(LineContentStr));
							// 清理行内容：去除前后空白和换行符
							TrackedLineContent = RawLineContent.TrimStartAndEnd();
						}
						else
						{
							TrackedLineContent = TEXT("<源码不可用>");
						}

						bSuccess = true;
						break;
					}
				}
			}

			// 清理引用（仅对 FrameSummary 获取的属性）
			if (strcmp(Py_TYPE(FrameInfo)->tp_name, "FrameSummary") == 0)
			{
				Py_XDECREF(FileNameObj);
				Py_XDECREF(LineNoObj);
				Py_XDECREF(FuncNameObj);
				Py_XDECREF(LineContentObj);
			}
		}
	}

	Py_XDECREF(Stack);
	Py_DECREF(ExtractStack);
	Py_DECREF(TracebackModule);
	ClearPythonErrors();

	return bSuccess;
}

bool FNePyTrackedObject::CaptureFromCppStack(int32 SkipFrames)
{
	// 简单的 C++ 调用栈捕获实现
	TrackedFileName = TEXT("C++ Code");
	TrackedLineNumber = -1;
	TrackedFunctionName = TEXT("Unknown C++ Function");
	TrackedLineContent = TEXT("<C++ 代码>");

	return true;
}

bool FNePyTrackedObject::ShouldSkipFileName(const FString& FileName) const
{
	return FileName.Contains(TEXT("<built-in>")) ||
		FileName.Contains(TEXT("<string>")) ||
		FileName.Contains(TEXT("<frozen")) ||
		FileName.EndsWith(TEXT(".pyd")) ||
		FileName.EndsWith(TEXT(".so")) ||
		FileName.EndsWith(TEXT(".dll"));
}

void FNePyTrackedObject::SetTrackingInfo(const FString& FileName, int32 LineNumber, const FString& FunctionName, const FString& LineContent)
{
	TrackedFileName = FileName;
	TrackedLineNumber = LineNumber;
	TrackedFunctionName = FunctionName;
	TrackedLineContent = LineContent.IsEmpty() ? TEXT("<手动设置>") : LineContent.TrimStartAndEnd();
	TrackedAllocationTime = FDateTime::Now();
	TrackedThreadId = FPlatformTLS::GetCurrentThreadId();
	bTrackingInfoValid = true;
}

void FNePyTrackedObject::ClearTrackingInfo()
{
	TrackedFileName.Empty();
	TrackedLineNumber = -1;
	TrackedFunctionName.Empty();
	TrackedLineContent.Empty();
	TrackedObjectTypeName.Empty();
	bTrackingInfoValid = false;
}

FString FNePyTrackedObject::GetTrackingInfoString() const
{
	if (!bTrackingInfoValid)
	{
		return TEXT("No tracking information available");
	}

	FString TypeInfo = TrackedObjectTypeName.IsEmpty() ? TEXT("Unknown") : TrackedObjectTypeName;
	FString DisplayFileName = FormatFilePathForDisplay(TrackedFileName);

	// 截断过长的行内容
	FString DisplayLineContent = TrackedLineContent;
	if (DisplayLineContent.Len() > 80)
	{
		DisplayLineContent = DisplayLineContent.Left(77) + TEXT("...");
	}

	return FString::Printf(TEXT("[%s #%llu] Created at %s:%d in %s()\n    Code: %s\n    [Thread: %u, Time: %s]"),
		*TypeInfo,
		TrackedObjectId,
		*DisplayFileName,
		TrackedLineNumber,
		*TrackedFunctionName,
		*DisplayLineContent,
		TrackedThreadId,
		*TrackedAllocationTime.ToString(TEXT("%H:%M:%S"))
	);
}

FString FNePyTrackedObject::GetShortTrackingInfo() const
{
	if (!bTrackingInfoValid)
	{
		return TEXT("Unknown");
	}

	return FString::Printf(TEXT("%s:%d"),
		*FormatFilePathForDisplay(TrackedFileName),
		TrackedLineNumber
	);
}

double FNePyTrackedObject::GetObjectLifetimeSeconds() const
{
	return (FDateTime::Now() - TrackedAllocationTime).GetTotalSeconds();
}

FString FNePyTrackedObject::FormatFilePathForDisplay(const FString& FilePath) const
{
	return FPaths::GetCleanFilename(FilePath);
}

void FNePyTrackedObject::ClearPythonErrors()
{
	if (PyErr_Occurred())
	{
		PyErr_Clear();
	}
}

// Python 接口实现
PyObject* FNePyTrackedObject::PyGetTrackingInfo(PyObject* Self)
{
	FNePyTrackedObject* TrackedObj = nullptr;
	if (Self)
	{
		// 尝试从子类获取 TrackedObject
		TrackedObj = reinterpret_cast<FNePyTrackedObject*>(Self);
	}

	if (!TrackedObj)
	{
		Py_RETURN_NONE;
	}

	FString InfoStr = TrackedObj->GetTrackingInfoString();

#if PY_MAJOR_VERSION >= 3
	return PyUnicode_FromString(TCHAR_TO_UTF8(*InfoStr));
#else
	return PyString_FromString(TCHAR_TO_UTF8(*InfoStr));
#endif
}

PyObject* FNePyTrackedObject::PyGetAllocationTime(PyObject* Self)
{
	FNePyTrackedObject* TrackedObj = reinterpret_cast<FNePyTrackedObject*>(Self);
	if (!TrackedObj)
	{
		Py_RETURN_NONE;
	}

	FString TimeStr = TrackedObj->TrackedAllocationTime.ToString();

#if PY_MAJOR_VERSION >= 3
	return PyUnicode_FromString(TCHAR_TO_UTF8(*TimeStr));
#else
	return PyString_FromString(TCHAR_TO_UTF8(*TimeStr));
#endif
}

PyObject* FNePyTrackedObject::PyHasTrackingInfo(PyObject* Self)
{
	FNePyTrackedObject* TrackedObj = reinterpret_cast<FNePyTrackedObject*>(Self);
	if (!TrackedObj)
	{
		Py_RETURN_FALSE;
	}

	return TrackedObj->bTrackingInfoValid ? Py_True : Py_False;
}

PyObject* FNePyTrackedObject::PyGetObjectTypeName(PyObject* Self)
{
	FNePyTrackedObject* TrackedObj = reinterpret_cast<FNePyTrackedObject*>(Self);
	if (!TrackedObj)
	{
		Py_RETURN_NONE;
	}

	FString TypeName = TrackedObj->GetObjectTypeName();
	if (TypeName.IsEmpty())
	{
		Py_RETURN_NONE;
	}

#if PY_MAJOR_VERSION >= 3
	return PyUnicode_FromString(TCHAR_TO_UTF8(*TypeName));
#else
	return PyString_FromString(TCHAR_TO_UTF8(*TypeName));
#endif
}

PyObject* FNePyTrackedObject::PyGetObjectId(PyObject* Self)
{
	FNePyTrackedObject* TrackedObj = reinterpret_cast<FNePyTrackedObject*>(Self);
	if (!TrackedObj)
	{
		return PyLong_FromLong(0);
	}

	return PyLong_FromUnsignedLongLong(TrackedObj->TrackedObjectId);
}

PyObject* FNePyTrackedObject::PyGetObjectLifetime(PyObject* Self)
{
	FNePyTrackedObject* TrackedObj = reinterpret_cast<FNePyTrackedObject*>(Self);
	if (!TrackedObj)
	{
		return PyFloat_FromDouble(0.0);
	}

	return PyFloat_FromDouble(TrackedObj->GetObjectLifetimeSeconds());
}

PyObject* FNePyTrackedObject::PyGetLineContent(PyObject* Self)
{
	FNePyTrackedObject* TrackedObj = reinterpret_cast<FNePyTrackedObject*>(Self);
	if (!TrackedObj)
	{
		Py_RETURN_NONE;
	}

	FString LineContent = TrackedObj->GetLineContent();
	if (LineContent.IsEmpty())
	{
		Py_RETURN_NONE;
	}

#if PY_MAJOR_VERSION >= 3
	return PyUnicode_FromString(TCHAR_TO_UTF8(*LineContent));
#else
	return PyString_FromString(TCHAR_TO_UTF8(*LineContent));
#endif
}

#endif // NEPY_ENABLE_PYTHON_OBJECT_TRACKING