#pragma once

#include <io.h>
#include <array>
#include <string>
#include <vector>
#include <direct.h>
#include <windows.h>

namespace path
{
	/**
	* 獲取當前目錄
	*/
	static std::wstring curdir()
	{
		wchar_t exe_full_path[MAX_PATH];
		GetModuleFileName(NULL, exe_full_path, MAX_PATH);
		std::wstring current_path(exe_full_path);
		int pos = current_path.find_last_of(L'\\', current_path.length());
		return current_path.substr(0, pos);
	}

	/**
	* 文件名分解
	*/
	static std::array<std::wstring, 2> splitext(const std::wstring &file_path)
	{
		auto pos = file_path.rfind('.');
		std::array<std::wstring, 2> text;
		if (std::wstring::npos != pos)
		{
			text[1] = file_path.substr(pos);
			text[0] = file_path.substr(0, pos);
		}
		else
		{
			text[0] = file_path;
		}
		return text;
	}

	/**
	* 列出子目錄下所有文件
	*/
	static std::vector<std::wstring> walk(const std::wstring &start_path)
	{
		_wfinddata_t file_info;
		std::vector<std::wstring> file_list;
		std::wstring find_path = start_path + L"\\*";
		long handle = _wfindfirst(find_path.c_str(), &file_info);

		if (handle == -1L) return file_list;

		do
		{
			if (file_info.attrib & _A_SUBDIR)
			{
				if ((wcscmp(file_info.name, L".") != 0) && (wcscmp(file_info.name, L"..") != 0))
				{
					std::wstring new_path = start_path + L"\\" + file_info.name;
					for (auto filename : walk(new_path)) file_list.push_back(filename);
				}
			}
			else
			{
				std::wstring new_path = start_path + L"\\";
				new_path += file_info.name;
				file_list.push_back(new_path);
			}
		} while (_wfindnext(handle, &file_info) == 0);

		_findclose(handle);

		return file_list;
	}
}