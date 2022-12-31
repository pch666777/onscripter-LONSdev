//windows平台上一些必要的准备

#include <Windows.h>
#include <string>


std::wstring GetLibDir() {
	wchar_t *buf = NULL;
	DWORD buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);

	buf = new wchar_t[buflen + 1];
	memset(buf, 0, sizeof(wchar_t) * (buflen + 1));
	buflen = GetCurrentDirectoryW(buflen, buf);

	std::wstring s(buf);
	delete[] buf;
	s.append(L"\\lib");
	return s;
}


void LoadLibs() {
	std::wstring s = GetLibDir();
	WIN32_FIND_DATA p;
	HANDLE h = FindFirstFile(s.c_str(), &p);
	while (h != INVALID_HANDLE_VALUE) {
		CloseHandle(h);
		LoadLibraryW((s + L"\\" + p.cFileName).c_str());
		if(! FindNextFileW(h, &p)) break;
	}
}