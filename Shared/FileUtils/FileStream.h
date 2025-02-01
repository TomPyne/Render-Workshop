#pragma once

#include <fstream>

enum class FileStreamMode_e
{
	READ,
	WRITE,
};

struct FileStream_s
{
	explicit FileStream_s(const std::wstring& Path, FileStreamMode_e InMode)
		: Mode(InMode)
	{
		if (Mode == FileStreamMode_e::READ)
		{
			InputStream = std::ifstream(Path, std::ios::in | std::ios::binary | std::ios::ate);
			Open = InputStream.is_open();
			if (Open)
			{
				Size = static_cast<size_t>(InputStream.tellg());
				InputStream.seekg(0, std::ios::beg);
			}
		}
		else
		{
			OutputStream = std::ofstream(Path, std::ios_base::out | std::ios::binary);
			Open = OutputStream.is_open();
		}
	}

	virtual ~FileStream_s()
	{
		if (InputStream.is_open())
			InputStream.close();

		if (OutputStream.is_open())
			OutputStream.close();
	}

	template<typename T>
	inline void ReadArray(T* Target, size_t Count)
	{
		if (Mode == FileStreamMode_e::READ)
		{
			InputStream.read(reinterpret_cast<char*>(Target), sizeof(T) * Count);
		}
	}

	template<typename T>
	inline void Read(T* Target)
	{
		ReadArray(Target, 1);
	}

	template<typename T>
	inline void WriteArray(const T* const Target, size_t Count)
	{
		if (Mode == FileStreamMode_e::WRITE)
		{
			OutputStream.write(reinterpret_cast<const char*>(Target), sizeof(T) * Count);
			Size += sizeof(T) * Count;
		}
	}

	template<typename T>
	inline void Write(const T* const Target)
	{
		WriteArray(Target, 1);
	}

	template<typename T>
	inline void StreamArray(T* Target, size_t Count)
	{
		switch (Mode)
		{
		case FileStreamMode_e::READ:
			ReadArray(Target, Count);
			break;
		case FileStreamMode_e::WRITE:
			WriteArray(Target, Count);
			break;
		}
	}

	template<typename T>
	inline void Stream(T* Target)
	{
		StreamArray(Target, 1);
	}

	template<typename T>
	inline void Read(std::vector<T>* TargetVector, uint32_t KnownSize = 0u)
	{
		if (KnownSize == 0)
		{
			Read(&KnownSize);
		}
		TargetVector->resize(KnownSize);
		ReadArray(TargetVector->data(), KnownSize);
	}

	template<typename T>
	inline void Write(std::vector<T>* TargetVector, uint32_t KnownSize = 0u)
	{
		if (KnownSize == 0u)
		{
			KnownSize = static_cast<uint32_t>(TargetVector->size());
			Write(&KnownSize);
		}
		WriteArray(TargetVector->data(), KnownSize);
	}

	template<typename T>
	inline void Stream(std::vector<T>* TargetVector, uint32_t KnownSize = 0u)
	{
		switch (Mode)
		{
		case FileStreamMode_e::READ:
			Read(TargetVector, KnownSize);
			break;
		case FileStreamMode_e::WRITE:
			Write(TargetVector, KnownSize);
			break;
		}
	}

	inline void Read(std::wstring* TargetStr, uint32_t KnownSize = 0u)
	{
		if (KnownSize == 0)
		{
			Read(&KnownSize);
		}
		TargetStr->resize(KnownSize);
		ReadArray(TargetStr->data(), KnownSize);
	}

	template<typename T>
	inline void Write(std::wstring* TargetStr, uint32_t KnownSize = 0u)
	{
		if (KnownSize == 0u)
		{
			KnownSize = static_cast<uint32_t>(TargetStr->size());
			Write(&KnownSize);
		}
		WriteArray(TargetStr->data(), KnownSize);
	}

	template<typename T>
	inline void Stream(std::wstring* TargetStr, uint32_t KnownSize = 0u)
	{
		switch (Mode)
		{
		case FileStreamMode_e::READ:
			Read(TargetStr, KnownSize);
			break;
		case FileStreamMode_e::WRITE:
			Write(TargetStr, KnownSize);
			break;
		}
	}

	inline bool IsOpen() const noexcept { return Open; }
	inline size_t GetSize() const noexcept { return Size; }

protected:
	std::ofstream OutputStream;
	std::ifstream InputStream;

	FileStreamMode_e Mode;

	bool Open = false;
	size_t Size = 0;
};

struct IFileStream_s : public FileStream_s
{
	explicit IFileStream_s(const std::wstring& Path)
		: FileStream_s(Path, FileStreamMode_e::READ)
	{
	}

	inline void Seek(size_t Offset)
	{
		InputStream.seekg(Offset);
	}
};

struct OFileStream_s : public FileStream_s
{
	explicit OFileStream_s(const std::wstring& Path)
		: FileStream_s(Path, FileStreamMode_e::WRITE)
	{
	}
};