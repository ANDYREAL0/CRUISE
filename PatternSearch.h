#pragma once
#include <Windows.h>
#include <vector>

namespace Process {
	HANDLE procHandle;
	void Initialize(int pid) {
		procHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	}

	//BYTE PlayerSign[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\x00\x01\x00\x00\x02";

	BYTE NameSign[] = "\x48\x8D\x15\x00\x00\x00\x00";
	BYTE OffsetSign[] = "\x41\xB9\x00\x00\x00\x00\x45\x33\xC0";

	string NameMask = "xxx????";
	string OffsetMask = "xx????xxx";

	DWORD64 FindPattern(BYTE* buffer, BYTE* pattern, string mask, int bufSize);

	vector<DWORD64> FindPatternEx(uintptr_t start, uintptr_t end, BYTE* pattern, string mask)
	{
		uintptr_t current_chunk = start;
		vector<uintptr_t> found;

		while (current_chunk < end)
		{
			int bufSize = (int)(end - start);
			BYTE* buffer = new BYTE[bufSize];
			if (!mem->RVM<uint64_t>(procHandle, (LPVOID)current_chunk, buffer, bufSize, nullptr))
			{
				current_chunk += bufSize;
				delete[] buffer;
				continue;
			}

			uintptr_t internal_address = FindPattern(buffer, pattern, mask, bufSize);
			if (internal_address != -1)
			{
				found.push_back(current_chunk + internal_address);
			}
			current_chunk += bufSize;
			delete[] buffer;

		}
		return found;
	}

	void test() {
		vector<uint64_t> TestAddress = FindPatternEx(GameData::BASE, 0xFFFFFFFFFF, NameSign, NameMask);
		//cout << TestAddress[0] << endl;
		uint64_t AddressReuslt = mem->RVM<uint64_t>(TestAddress[0]);
		cout << AddressReuslt << endl;
		char buff[] = "";
		mem->readSTR(AddressReuslt, buff, sizeof(char));
		for (int i = 0; i < sizeof(buff); i++) {
			cout << buff;
		}
	}

	DWORD64 FindPattern(BYTE* buffer, BYTE* pattern, string mask, int bufSize)
	{
		int pattern_len = mask.length();
		for (int i = 0; i < bufSize - pattern_len; i++)
		{
			bool found = true;
			for (int j = 0; j < pattern_len; j++)
			{
				if (mask[j] != '?' && pattern[j] != buffer[(i + j)])
				{
					found = false;
					break;
				}
			}
			if (found)
				return i;
		}
		return -1;
	}
};