#include "CngMd5.h"
#include "net/Logger.h"

#if defined(WIN32) || defined(_WIN32)
#pragma comment(lib, "Bcrypt.lib")
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

using namespace xop;

CngMd5::CngMd5() : Md5()
{

#if defined(WIN32) || defined(_WIN32)
	DWORD cbData = 0;
	NTSTATUS status;
	if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(
				&hAlgorithm_, BCRYPT_MD5_ALGORITHM, nullptr, 0))) {
		LOG_ERROR(
			"**** Error 0x%x returned by BCryptOpenAlgorithmProvider",
			status);
		return;
	}
	if (!NT_SUCCESS(status = BCryptGetProperty(
				hAlgorithm_, BCRYPT_OBJECT_LENGTH,
				(PBYTE)&cbHashObject_, sizeof(DWORD), &cbData,
				0))) {
		LOG_ERROR("**** Error 0x%x returned by BCryptGetProperty",
			  status);
		CngMd5::~CngMd5();
		return;
	}
	if (!NT_SUCCESS(status = BCryptGetProperty(
				hAlgorithm_, BCRYPT_HASH_LENGTH,
				(PBYTE)&cbHash_, sizeof(DWORD), &cbData, 0))) {
		LOG_ERROR("**** Error 0x%x returned by BCryptGetProperty",
			  status);
		CngMd5::~CngMd5();
		return;
	}
	if (cbHash_ > MD5_HASH_LENGTH) {
		LOG_ERROR("**** The generated hash value is too long");
		CngMd5::~CngMd5();
	}
#endif
}

CngMd5::~CngMd5()
{
#if defined(WIN32) || defined(_WIN32)
	if (hAlgorithm_) {
		BCryptCloseAlgorithmProvider(hAlgorithm_, 0);
		hAlgorithm_ = nullptr;
	}
#endif
}

void CngMd5::GetMd5Hash(const unsigned char *data, const size_t dataSize,
			unsigned char *outHash)
{
#if defined(WIN32) || defined(_WIN32)
	if (hAlgorithm_ == nullptr) return;

	const auto pbHashObject = static_cast<PBYTE>(
		HeapAlloc(GetProcessHeap(), 0, cbHashObject_));
	//create a hash
	BCRYPT_HASH_HANDLE hHash = nullptr;

	auto cleanup = [&] () {
		if (hHash)
			BCryptDestroyHash(hHash);
		if (pbHashObject)
			HeapFree(GetProcessHeap(), 0, pbHashObject);
	};

	if (nullptr == pbHashObject) {
		LOG_ERROR("**** memory allocation failed");
		cleanup();
		return;
	}
	NTSTATUS status;
	if (!NT_SUCCESS(status = BCryptCreateHash(hAlgorithm_, &hHash,
						  pbHashObject, cbHashObject_,
						  nullptr, 0, 0))) {
		LOG_ERROR("**** Error 0x%x returned by BCryptCreateHash",
			  status);
		cleanup();
		return;
	}

	if (!NT_SUCCESS(status = BCryptHashData(hHash, const_cast<PBYTE>(data),
						static_cast<ULONG>(dataSize), 0))) {
		LOG_ERROR("**** Error 0x%x returned by BCryptHashData", status);
		cleanup();
		return;
	}

	//close the hash
	if (!NT_SUCCESS(status =
				BCryptFinishHash(hHash, outHash, cbHash_, 0))) {
		LOG_ERROR("**** Error 0x%x returned by BCryptFinishHash",
			  status);
	}
	cleanup();
#endif
}
