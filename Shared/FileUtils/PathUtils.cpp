#include "PathUtils.h"

#include "Logging/Logging.h"
#include <filesystem>

std::wstring GetPathExtension(const wchar_t* Path)
{
	CHECK(Path);

	wchar_t Ext[_MAX_EXT] = {};
	_wsplitpath_s(Path, nullptr, 0, nullptr, 0, nullptr, 0, Ext, _MAX_EXT);
	return Ext;
}

std::wstring ReplacePathExtension(const wchar_t* Path, const wchar_t* NewExtension)
{
	CHECK(Path);
	CHECK(NewExtension);

	wchar_t Drive[_MAX_DRIVE] = {};
	wchar_t Dir[_MAX_DIR] = {};
	wchar_t FName[_MAX_FNAME] = {};
	_wsplitpath_s(Path, Drive, _MAX_DRIVE, Dir, _MAX_DIR, FName, _MAX_FNAME, nullptr, 0);

	wchar_t NewPath[PathUtils::MaxPath] = {};
	_wmakepath_s(NewPath, PathUtils::MaxPath, Drive, Dir, FName, NewExtension);

	return NewPath;
}

bool HasPathExtension(const wchar_t* Path, const wchar_t* Extension)
{
	CHECK(Path);
	CHECK(Extension);

	wchar_t Ext[_MAX_EXT] = {};
	_wsplitpath_s(Path, nullptr, 0, nullptr, 0, nullptr, 0, Ext, _MAX_EXT);

	return wcscmp(Ext, Extension) == 0;
}

std::wstring MakePathAbsolute(const std::wstring& path)
{
	return std::filesystem::absolute(path).native();
}

std::wstring MakePathRelativeTo(const std::wstring& Path, const std::wstring& Base)
{
	return std::filesystem::relative(Path, Base).native();
}

bool CreateDirectories(const std::wstring& Path)
{
	std::filesystem::path ParentPath = std::filesystem::path(Path).parent_path();
	return std::filesystem::create_directories(ParentPath);
}
