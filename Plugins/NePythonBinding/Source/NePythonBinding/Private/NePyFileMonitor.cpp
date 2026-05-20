#include "NePyFileMonitor.h"

#if PLATFORM_WINDOWS

#include "NePyGIL.h"
#include "NePyPtr.h"
#include "Windows/AllowWindowsPlatformTypes.h"

#include <utility>
#include <map>
#include <string>
#include <vector>


static DWORD WINAPI FmMonitorThread(LPVOID arg)
{
	std::pair<HANDLE, PyObject*>* HandleCallbackPair = static_cast<std::pair<HANDLE, PyObject*>*>(arg);
	HANDLE DirectoryHandle = HandleCallbackPair->first;
	PyObject* PyCallback = HandleCallbackPair->second;
	char Buffer[MAX_PATH * 256];
	DWORD BytesReturned;
	while (TRUE)
	{
		ReadDirectoryChangesW(
			DirectoryHandle,
			&Buffer,
			sizeof(Buffer),
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_CREATION,
			&BytesReturned,
			nullptr,
			nullptr);

		std::vector<FILE_NOTIFY_INFORMATION*> Notifies;
		{
			FILE_NOTIFY_INFORMATION* Notify = (FILE_NOTIFY_INFORMATION*)Buffer;
			while (Notify)
			{
				Notifies.push_back(Notify);
				if (Notify->NextEntryOffset == 0)
				{
					break;
				}
				Notify = (FILE_NOTIFY_INFORMATION*)((char*)Notify + Notify->NextEntryOffset);
			}
		}

		{
			FNePyScopedGIL GIL;
			PyObject* PyResult = PyTuple_New(Notifies.size());
			size_t Index = 0;
			for (FILE_NOTIFY_INFORMATION* Notify : Notifies)
			{
				PyObject* PyItem = PyTuple_New(2);
#if PY_MAJOR_VERSION >= 3
				PyTuple_SetItem(PyItem, 0, PyUnicode_FromWideChar(Notify->FileName, Notify->FileNameLength / sizeof(WCHAR)));
				PyTuple_SetItem(PyItem, 1, PyLong_FromLong(Notify->Action));
#else
				int LenWchar = Notify->FileNameLength / sizeof(WCHAR);
				int LenChar = WideCharToMultiByte(CP_ACP, 0, Notify->FileName, LenWchar, nullptr, 0, nullptr, nullptr);
				std::string FileName(LenChar, 0);
				WideCharToMultiByte(CP_ACP, 0, Notify->FileName, LenWchar, &FileName[0], LenChar, nullptr, nullptr);
				PyTuple_SetItem(PyItem, 0, NePyString_FromString(FileName.c_str()));
				PyTuple_SetItem(PyItem, 1, PyInt_FromLong(Notify->Action));
#endif
				PyTuple_SetItem(PyResult, Index, PyItem);
				Index++;
			}
			FNePyObjectPtr PyArgList = NePyStealReference(PyTuple_New(1));
			PyTuple_SetItem(PyArgList, 0, PyResult);
			FNePyObjectPtr PyRetVal = NePyStealReference(PyObject_Call(PyCallback, PyArgList, nullptr));
			if (!PyRetVal)
			{
				PyErr_Print();
			}
		}
	}
	return 0;
}

static void FmMonitorDirectoryChanges(const std::string& Directory, PyObject* PyCallback)
{
	static std::map<std::string, std::pair<HANDLE, PyObject*>> DirectoryHandleMap;
	if (DirectoryHandleMap.find(Directory) == DirectoryHandleMap.end())
	{
		HANDLE Handle = ::CreateFileA(Directory.c_str(), FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr);

		if (Handle == INVALID_HANDLE_VALUE)
		{
			return;
		}
		// i am not going to release it
		Py_XINCREF(PyCallback);
		DirectoryHandleMap[Directory] = std::pair<HANDLE, PyObject*>(Handle, PyCallback);

		DWORD ThreadId = 0;
		CreateThread(nullptr, 0, FmMonitorThread, &DirectoryHandleMap[Directory], 0, &ThreadId);
	}
}

static PyObject* FmMonitorDirectoryChanges(PyObject* InSelf, PyObject* InArgs)
{
	char* Directory;
	PyObject* PyCallback;
	if (!PyArg_ParseTuple(InArgs, "sO", &Directory, &PyCallback))
	{
		return nullptr;
	}
	if (!PyCallable_Check(PyCallback))
	{
		PyErr_SetString(PyExc_TypeError, "argument 2 must be callable");
		return nullptr;
	}
	FmMonitorDirectoryChanges(Directory, PyCallback);
	Py_RETURN_NONE;
}

static PyObject* FmIsSymbolicLink(PyObject* InSelf, PyObject* InArgs)
{
	char* Directory;
	if (!PyArg_ParseTuple(InArgs, "s", &Directory))
	{
		return nullptr;
	}
	if (GetFileAttributesA(Directory) & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

static PyMethodDef  PyFileMonitorMethods[] = {
   {"FmMonitorDirectoryChanges", FmMonitorDirectoryChanges, METH_VARARGS, "monitor Directory changes"},
   {"FmIsSymbolicLink", FmIsSymbolicLink, METH_VARARGS, "A file or Directory that has an associated reparse point, or a file that is a symbolic link" },
   {nullptr}
};

void NePyInitFileMonitor(PyObject* PyOuterModule)
{
	PyObject* PyModuleDict = PyModule_GetDict(PyOuterModule);

	PyMethodDef* PyModuleMethod;
	for (PyModuleMethod = PyFileMonitorMethods; PyModuleMethod->ml_name != nullptr; PyModuleMethod++)
	{
		PyObject* PyFunc = PyCFunction_New(PyModuleMethod, nullptr);
		PyDict_SetItemString(PyModuleDict, PyModuleMethod->ml_name, PyFunc);
		Py_DECREF(PyFunc);
	}
}

#include "Windows/HideWindowsPlatformTypes.h"

#endif
