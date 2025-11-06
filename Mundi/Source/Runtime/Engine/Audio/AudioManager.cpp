// Copyright Mundi Game Engine. All Rights Reserved.

#include "pch.h"
#include "AudioManager.h"
#include <filesystem>
#include <Windows.h>
#include <fstream>

// WAV 파일 헤더 구조체
struct FWavHeader
{
	char ChunkID[4];			// "RIFF"
	DWORD ChunkSize;
	char Format[4];				// "WAVE"
};

struct FWavFormatChunk
{
	char SubchunkID[4];			// "fmt "
	DWORD SubchunkSize;
	WORD AudioFormat;
	WORD NumChannels;
	DWORD SampleRate;
	DWORD ByteRate;
	WORD BlockAlign;
	WORD BitsPerSample;
};

struct FWavDataChunkHeader
{
	char SubchunkID[4];			// "data"
	DWORD SubchunkSize;
};

UAudioManager::UAudioManager()
{
}

UAudioManager::~UAudioManager()
{
	Shutdown();
}

UAudioManager& UAudioManager::GetInstance()
{
	static UAudioManager Instance;
	return Instance;
}

void UAudioManager::Initialize()
{
	if (bIsInitialized)
	{
		UE_LOG("AudioManager already initialized.");
		return;
	}

	// COM 초기화
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		UE_LOG("Failed to initialize COM: 0x%08X", hr);
		return;
	}

	// XAudio2 생성
	hr = XAudio2Create(&XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		UE_LOG("Failed to create XAudio2: 0x%08X", hr);
		CoUninitialize();
		return;
	}

	// MasteringVoice 생성
	hr = XAudio2->CreateMasteringVoice(&MasteringVoice);
	if (FAILED(hr))
	{
		UE_LOG("Failed to create MasteringVoice: 0x%08X", hr);
		XAudio2->Release();
		XAudio2 = nullptr;
		CoUninitialize();
		return;
	}

	UE_LOG("XAudio2 initialized successfully.");


	LoadAllSounds();

	bIsInitialized = true;
}

void UAudioManager::Update(float DeltaTime)
{
	// XAudio2는 별도 업데이트가 필요 없음 (자동으로 처리됨)
}

void UAudioManager::Shutdown()
{
	if (!bIsInitialized)
		return;

	// 모든 WAV 데이터 해제
	WavDataMap.clear();
	SoundFilePaths.Empty();

	// MasteringVoice 해제
	if (MasteringVoice)
	{
		MasteringVoice->DestroyVoice();
		MasteringVoice = nullptr;
	}

	// XAudio2 해제
	if (XAudio2)
	{
		XAudio2->Release();
		XAudio2 = nullptr;
	}

	// COM 해제
	CoUninitialize();

	bIsInitialized = false;
	UE_LOG("AudioManager shutdown complete.");
}

void UAudioManager::LoadAllSounds()
{
	if (!XAudio2)
	{
		UE_LOG("XAudio2 not initialized. Cannot load sounds.");
		return;
	}


	// 기존 데이터 클리어
	WavDataMap.clear();
	SoundFilePaths.Empty();

	// Data/Sound 폴더 경로
	FWideString SoundFolderPath = L"Data/Sound";

	// 폴더가 존재하는지 확인
	if (!std::filesystem::exists(SoundFolderPath))
	{
		UE_LOG("Sound folder not found: Data/Sound");
		return;
	}

	// 재귀적으로 .wav 파일 검색
	TArray<FWideString> WavFiles;
	ScanFolderRecursive(SoundFolderPath, WavFiles);


	// 각 .wav 파일 로드
	for (const FWideString& FilePath : WavFiles)
	{
		// Wide string을 UTF-8 FString으로 변환
		int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, FilePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
		FString FilePathUTF8;
		FilePathUTF8.resize(SizeNeeded - 1);
		WideCharToMultiByte(CP_UTF8, 0, FilePath.c_str(), -1, &FilePathUTF8[0], SizeNeeded, nullptr, nullptr);

		// 경로 정규화 (백슬래시를 슬래시로 변환)
		std::replace(FilePathUTF8.begin(), FilePathUTF8.end(), '\\', '/');

		// WAV 데이터 로드
		auto WavData = std::make_unique<FWavData>();
		if (LoadWavFile(FilePath, WavData.get()))
		{
			WavDataMap[FilePathUTF8] = std::move(WavData);
			SoundFilePaths.push_back(FilePathUTF8);
			UE_LOG("Loaded sound: %s", FilePathUTF8.c_str());
		}
		else
		{
			UE_LOG("Failed to load sound: %s", FilePathUTF8.c_str());
		}
	}

	UE_LOG("Loaded %d sound files from Data/Sound folder.", SoundFilePaths.size());
}

void UAudioManager::ScanFolderRecursive(const FWideString& FolderPath, TArray<FWideString>& OutWavFiles)
{
	// Windows API를 사용하여 폴더 스캔
	FWideString SearchPath = FolderPath + L"/*";
	WIN32_FIND_DATAW FindData;
	HANDLE FindHandle = FindFirstFileW(SearchPath.c_str(), &FindData);

	if (FindHandle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{
		FWideString FileName = FindData.cFileName;

		// "." 및 ".." 무시
		if (FileName == L"." || FileName == L"..")
			continue;

		FWideString FullPath = FolderPath + L"/" + FileName;

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// 하위 폴더 재귀 탐색
			ScanFolderRecursive(FullPath, OutWavFiles);
		}
		else
		{
			// .wav 파일인지 확인
			if (FileName.size() >= 4)
			{
				FWideString Extension = FileName.substr(FileName.size() - 4);
				std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::towlower);

				if (Extension == L".wav")
				{
					OutWavFiles.push_back(FullPath);
				}
			}
		}
	} while (FindNextFileW(FindHandle, &FindData));

	FindClose(FindHandle);
}

bool UAudioManager::LoadWavFile(const FWideString& FilePath, FWavData* OutWavData)
{
	if (!OutWavData)
		return false;

	// 파일 열기
	std::ifstream File(FilePath, std::ios::binary);
	if (!File.is_open())
	{
		UE_LOG("Failed to open WAV file: %ws", FilePath.c_str());
		return false;
	}

	// RIFF 헤더 읽기
	FWavHeader Header;
	File.read(reinterpret_cast<char*>(&Header), sizeof(FWavHeader));
	if (strncmp(Header.ChunkID, "RIFF", 4) != 0 || strncmp(Header.Format, "WAVE", 4) != 0)
	{
		UE_LOG("Invalid WAV file format: %ws", FilePath.c_str());
		return false;
	}

	// fmt 청크 읽기
	FWavFormatChunk FormatChunk;
	File.read(reinterpret_cast<char*>(&FormatChunk), sizeof(FWavFormatChunk));
	if (strncmp(FormatChunk.SubchunkID, "fmt ", 4) != 0)
	{
		UE_LOG("Invalid fmt chunk in WAV file: %ws", FilePath.c_str());
		return false;
	}

	// WAVEFORMATEX 채우기
	OutWavData->WaveFormat.wFormatTag = FormatChunk.AudioFormat;
	OutWavData->WaveFormat.nChannels = FormatChunk.NumChannels;
	OutWavData->WaveFormat.nSamplesPerSec = FormatChunk.SampleRate;
	OutWavData->WaveFormat.nAvgBytesPerSec = FormatChunk.ByteRate;
	OutWavData->WaveFormat.nBlockAlign = FormatChunk.BlockAlign;
	OutWavData->WaveFormat.wBitsPerSample = FormatChunk.BitsPerSample;
	OutWavData->WaveFormat.cbSize = 0;

	// fmt 청크의 추가 데이터 건너뛰기 (16바이트보다 큰 경우)
	if (FormatChunk.SubchunkSize > 16)
	{
		File.seekg(FormatChunk.SubchunkSize - 16, std::ios::cur);
	}

	// data 청크 찾기
	FWavDataChunkHeader DataChunkHeader;
	while (File.read(reinterpret_cast<char*>(&DataChunkHeader), sizeof(FWavDataChunkHeader)))
	{
		if (strncmp(DataChunkHeader.SubchunkID, "data", 4) == 0)
		{
			break;
		}
		// data가 아닌 다른 청크는 건너뛰기
		File.seekg(DataChunkHeader.SubchunkSize, std::ios::cur);
	}

	if (strncmp(DataChunkHeader.SubchunkID, "data", 4) != 0)
	{
		UE_LOG("data chunk not found in WAV file: %ws", FilePath.c_str());
		return false;
	}

	// 오디오 데이터 읽기
	OutWavData->AudioDataSize = DataChunkHeader.SubchunkSize;
	OutWavData->AudioData = new BYTE[OutWavData->AudioDataSize];
	File.read(reinterpret_cast<char*>(OutWavData->AudioData), OutWavData->AudioDataSize);

	File.close();
	return true;
}

FWavData* UAudioManager::GetSound(const FString& FilePath)
{
	// 경로 정규화
	FString NormalizedPath = FilePath;
	std::replace(NormalizedPath.begin(), NormalizedPath.end(), '\\', '/');

	auto It = WavDataMap.find(NormalizedPath);
	if (It != WavDataMap.end())
	{
		return It->second.get();
	}

	return nullptr;
}

void UAudioManager::SetMasterVolume(float Volume)
{
	MasterVolume = std::clamp(Volume, 0.0f, 100.0f);

	if (MasteringVoice)
	{
		// XAudio2는 0.0 ~ 1.0 범위를 사용하므로 변환
		float NormalizedVolume = MasterVolume / 100.0f;
		MasteringVoice->SetVolume(NormalizedVolume);
	}
}
