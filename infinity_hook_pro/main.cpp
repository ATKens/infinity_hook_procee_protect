#include "hook.hpp"

EXTERN_C VOID AsmInt3();

typedef NTSTATUS(*FNtCreateFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
FNtCreateFile g_NtCreateFile = 0;


typedef NTSTATUS(*SaveNtOpenProcess)(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
SaveNtOpenProcess g_NtOpenProcess = 0;

NTSTATUS MyNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
	if (ObjectAttributes &&
		ObjectAttributes->ObjectName &&
		ObjectAttributes->ObjectName->Buffer)
	{
		wchar_t* name = (wchar_t*)ExAllocatePool(NonPagedPool, ObjectAttributes->ObjectName->Length + sizeof(wchar_t));
		if (name)
		{
			RtlZeroMemory(name, ObjectAttributes->ObjectName->Length + sizeof(wchar_t));
			RtlCopyMemory(name, ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length);

			if (wcsstr(name, L"tips.txt"))
			{
				ExFreePool(name);
				return STATUS_ACCESS_DENIED;
			}

			ExFreePool(name);
		}
	}

	return g_NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}


NTSTATUS FakeNtOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId)
{
	if (ClientId->UniqueProcess == (HANDLE)0x904)
	{
		KdPrint(("Target process is being opened.!!!!!!!!!!!!-_-_-_-_-____----___"));
		return STATUS_ACCESS_DENIED;
	}

	return g_NtOpenProcess( ProcessHandle,  DesiredAccess,  ObjectAttributes,  ClientId);
}




void __fastcall call_back(unsigned long ssdt_index, void** ssdt_address)
{
	UNREFERENCED_PARAMETER(ssdt_index);

	if (*ssdt_address == g_NtCreateFile) *ssdt_address = MyNtCreateFile;
}



void __fastcall call_back2(unsigned long ssdt_index, void** ssdt_address)
{
	UNREFERENCED_PARAMETER(ssdt_index);

	if (*ssdt_address == g_NtOpenProcess) *ssdt_address = FakeNtOpenProcess;
}



VOID DriverUnload(PDRIVER_OBJECT driver)
{
	UNREFERENCED_PARAMETER(driver);

	k_hook::stop();
}

EXTERN_C
NTSTATUS
DriverEntry(
	PDRIVER_OBJECT driver,
	PUNICODE_STRING registe)
{
	UNREFERENCED_PARAMETER(registe);

	driver->DriverUnload = DriverUnload;

	/*
	UNICODE_STRING str;
	WCHAR name[256]{ L"NtCreateFile" };
	RtlInitUnicodeString(&str, name);
	g_NtCreateFile = (FNtCreateFile)MmGetSystemRoutineAddress(&str);

	// 初始化并挂钩
	return k_hook::initialize(call_back) && k_hook::start() ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	*/


	
#if DBG
	AsmInt3();
#endif
	//test NtOpenprocess
	UNICODE_STRING str;
	WCHAR name[256]{L"NtOpenProcess"};
	RtlInitUnicodeString(&str, name);
	g_NtOpenProcess = (SaveNtOpenProcess)MmGetSystemRoutineAddress(&str);

	// 初始化并挂钩
	return k_hook::initialize(call_back2) && k_hook::start() ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

	
}