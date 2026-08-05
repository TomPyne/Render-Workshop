#include "PathUtils.h"

#include "Logging/Logging.h"
#include "StringUtils/StringUtils.h"

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

std::wstring Path_s::DefaultProjectName;

Path_s::Path_s(const std::wstring& InPath)
{
	if (ENSUREMSG(!DefaultProjectName.empty(), "[Path_s] Default project name is not set, cannot construct path"))
	{
		Path = ConstructPathPrefix(PathDirectory_e::Root, DefaultProjectName) + InPath;
	}
}

Path_s::Path_s(PathDirectory_e InDirectory, const std::wstring& InPath)
{
	if (ENSUREMSG(!DefaultProjectName.empty(), "[Path_s] Default project name is not set, cannot construct path"))
	{
		Path = ConstructPathPrefix(InDirectory, DefaultProjectName) + InPath;
	}	
}

Path_s::Path_s(PathDirectory_e InDirectory, const std::wstring& InProject, const std::wstring& InPath)
{
	Path = ConstructPathPrefix(InDirectory, InProject) + InPath;
}

std::wstring Path_s::ToWString() const
{
	return Path;
}

std::string Path_s::ToString() const
{
	return WideToNarrow(Path);
}

void Path_s::SetDefaultProject(const std::wstring& InProjectName)
{
	DefaultProjectName = InProjectName;
}

std::wstring Path_s::ConstructPathPrefix(PathDirectory_e Directory, const std::wstring& Project)
{
	switch (Directory)
	{
	case PathDirectory_e::Root:
		return L"";
	case PathDirectory_e::Project:
		return Project + L"/";
	case PathDirectory_e::Assets:
		return Project + L"/Assets/";
	case PathDirectory_e::Shaders:
		return Project + L"/Shaders/";
	default:
		LOGERROR("[Path_s] Invalid directory enum value");
		return L"";
	}
}
