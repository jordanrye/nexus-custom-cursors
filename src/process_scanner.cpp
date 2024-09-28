#include <Windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <Strsafe.h>
#pragma warning (push)
#pragma warning (disable: 4201)
#include <winternl.h>
#pragma warning (pop)
#include <TlHelp32.h>

#include "process_scanner.h"

HANDLE g_process;
HANDLE g_window;
DWORD g_id;

void process_find_window(i8 const* wnd, i8 const* cls)
{
	static i8 swnd[256] = { 0 };
	static i8 scls[256] = { 0 };

	if (wnd) StringCchCopyA(swnd, sizeof(swnd), wnd);
	if (cls) StringCchCopyA(scls, sizeof(scls), cls);

	if (swnd[0] == 0 || scls[0] == 0)
		return;

	g_window = FindWindowA(scls, swnd);
	if (!g_window)
	{

	}
	else
	{

	}
}

void process_create(i8 const* wnd, i8 const* cls)
{
	g_id = GetCurrentProcessId();
	g_process = GetCurrentProcess();

	// Get the game window handle
	process_find_window(wnd, cls);
}

u64 process_follow(i8 const* pattern, i8 const* mask, i32 off)
{
	u64 addr = process_scan(pattern, mask);
	if (addr == 0)
	{
		return 0;
	}

	return process_follow_rel_addr(addr + off);
}

void* process_get_handle(void)
{
	return g_process;
}

u32 process_get_id(void)
{
	return g_id;
}

void* process_get_window(void)
{
	if (!g_window)
		process_find_window(NULL, NULL);

	return g_window;
}

bool process_read(u64 base, void* dst, u32 bytes)
{
	if (!base || !dst)
		return false;

	if (ReadProcessMemory(g_process, (void*)base, dst, bytes, 0) == FALSE)
	{
		memset(dst, 0, bytes);
		return false;
	}

	return true;
}

u64 process_scan(void const* sig, i8 const* msk)
{
	u32 bytes = 0;
	while (msk[bytes])
	{
		++bytes;
	}

	u64 addr = 0;
	MEMORY_BASIC_INFORMATION mbi;

	while (VirtualQuery((void*)addr, &mbi, sizeof(mbi)))
	{
		addr += mbi.RegionSize;

		if (mbi.RegionSize < bytes ||
			mbi.State != MEM_COMMIT ||
			mbi.Protect != PAGE_EXECUTE_READ)
		{
			continue;
		}

		u8* s1 = (u8*)sig;
		u8* buf = (u8*)mbi.BaseAddress;
		u64 end = mbi.RegionSize - bytes;

		for (u64 i = 0; i < end; ++i)
		{
			bool match = true;

			for (u64 j = 0; j < bytes; ++j)
			{
				if (msk[j] == 'x' && buf[i + j] != s1[j])
				{
					match = false;
					break;
				}
			}

			if (match)
			{
				return (u64)mbi.BaseAddress + i;
			}
		}
	}

	return 0;
}

uintptr_t process_follow_rel_addr(uintptr_t addr)
{
	return *(int32_t*)addr + addr + 4;
}
