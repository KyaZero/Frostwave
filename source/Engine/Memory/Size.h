#pragma once
#include <Engine/Core/Types.h>

class Size
{
public:
	Size() = delete;
	explicit Size(u64 bytes) { m_SizeInBytes = bytes; }
	~Size() {}

	u64 AsBytes() const { return m_SizeInBytes; }
	f64 AsKiloBytes() const { return AsBytes() / 1024.0; };
	f64 AsMegabytes() const { return AsKiloBytes() / 1024.0; };
	f64 AsGigabytes() const { return AsMegabytes() / 1024.0; };

	operator size_t() const { return AsBytes(); }

	static Size Bytes(u64 bytes) { return Size(bytes); }
	static Size Kilobytes(u64 bytes) { return Bytes(bytes * 1024); }
	static Size Megabytes(u64 bytes) { return Kilobytes(bytes * 1024); }
	static Size Gigabytes(u64 bytes) { return Megabytes(bytes * 1024); }
private:
	u64 m_SizeInBytes;
};

// supress warning for reserved postfix operators
#pragma warning(disable : 4455)
static inline Size operator ""_B(u64 bytes) { return Size::Bytes((u64)bytes); }
static inline Size operator ""KB(u64 bytes) { return Size::Kilobytes((u64)bytes); }
static inline Size operator ""MB(u64 bytes) { return Size::Megabytes((u64)bytes); }
static inline Size operator ""GB(u64 bytes) { return Size::Gigabytes((u64)bytes); }
#pragma warning(default : 4455)
