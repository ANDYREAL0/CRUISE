#pragma once
#include <iostream>
#include <basetsd.h>
#include <list>
#include <d3d9.h>
#include <d3dx9.h>
#include "offsets.h"

#define INRANGE(x,a,b)		(x >= a && x <= b) 
#define getBits( x )		(INRANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xa))
#define getByte( x )		(getBits(x[0]) << 4 | getBits(x[1]))

// Request to retrieve initialize from kernel space
#define IO_Init_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5831 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to retrieve the base address of process in csgo.exe from kernel space
#define IO_GET_MODULE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5832 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to read virtual user memory (memory of a program) from kernel space
#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5833 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to write virtual user memory (memory of a program) from kernel space
#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5834 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to protect a process from kernel space
#define IO_PROTECT_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5835 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define RegPath "\\\\.\\f879eo7athgf0asof76t3qhhog"

typedef BOOL(WINAPI* reserve_init)();

typedef BOOL(WINAPI* reserve_module)
(
	ULONG uProc,
	LPCSTR sModule,
	ULONGLONG* phmModule,
	ULONG* puModSize
	);

typedef BOOL(WINAPI* reserve_alloc)
(
	ULONG uProc,
	ULONGLONG& pbAddr,
	ULONG nSize,
	ULONG uProtect
	);

typedef BOOL(WINAPI* reserve_free)
(
	ULONG uProc,
	ULONGLONG pbAddr,
	ULONG nSize
	);

typedef BOOL(WINAPI* reserve_read)
(
	ULONG uProc,
	ULONGLONG pbAddr,
	PVOID pbData,
	ULONG uSize
	);

typedef BOOL(WINAPI* reserve_write)
(
	ULONG uProc,
	ULONGLONG pbAddr,
	PVOID pbData,
	ULONG uSize
	);

typedef struct _KERNEL_READ_REQUEST
{
	DWORD_PTR TargetAddress;
	DWORD_PTR ResponseAddress;
	ULONG Size;
} KERNEL_READ_REQUEST, * PKERNEL_READ_REQUEST;

typedef struct _KERNEL_WRITE_REQUEST
{
	DWORD_PTR TargetAddress;
	DWORD_PTR CopiedFromAddress;
	ULONG Size;
} KERNEL_WRITE_REQUEST, * PKERNEL_WRITE_REQUEST;

//typedef struct _KERNEL_FORCEWRITE_REQUEST
//{
//	DWORD_PTR TargetAddress;
//	DWORD_PTR CopiedFromAddress;
//	ULONG Size;
//} KERNEL_FORCEWRITE_REQUEST, * PKERNEL_FORCEWRITE_REQUEST;

namespace KernelDriver
{

	void WINAPI Init(LPCSTR modulename, LPCWSTR processname);

	BOOL WINAPI LoadDriver();

	DWORD_PTR WINAPI DriverGetModulebase(OPTIONAL ULONG size);
	BOOL WINAPI DriverReadMemory(_In_  ULONGLONG lpBaseAddress, _Out_ LPVOID  lpBuffer, _In_  SIZE_T  nSize);
	BOOL WINAPI DriverWriteMemory(_In_  ULONGLONG  lpBaseAddress, _In_  LPVOID lpBuffer, _In_  SIZE_T  nSize);
	//BOOL WINAPI DriverForceAllocate(_In_  ULONGLONG  lpBaseAddress, _In_  LPVOID lpBuffer, _In_  SIZE_T  nSize);

	extern HANDLE hDriver;
	extern ULONG ProcID;

};
