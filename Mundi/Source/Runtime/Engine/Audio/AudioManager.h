// Copyright Mundi Game Engine. All Rights Reserved.

#pragma once
#include <memory>
#include <xaudio2.h>

// WAV 파일 데이터를 담는 구조체
struct FWavData
{
	WAVEFORMATEX WaveFormat;
	BYTE* AudioData = nullptr;
	DWORD AudioDataSize = 0;

	~FWavData()
	{
		if (AudioData)
		{
			delete[] AudioData;
			AudioData = nullptr;
		}
	}
};

/**
 * UAudioManager
 *
 * 게임 엔진의 오디오 시스템을 관리하는 싱글톤 클래스
 * - XAudio2 기반 오디오 엔진
 * - Data/Sound 폴더의 모든 .wav 파일을 미리 로드
 * - AudioComponent에서 사운드 파일 선택 및 재생을 위한 인터페이스 제공
 */
class UAudioManager
{
public:
	// --- 싱글톤 접근자 ---
	static UAudioManager& GetInstance();

	// --- 초기화 및 수명 주기 ---
	void Initialize();
	void Update(float DeltaTime);
	void Shutdown();

	// --- 사운드 리소스 관리 ---
	/**
	 * Data/Sound 폴더를 재귀적으로 스캔하여 모든 .wav 파일을 로드합니다.
	 */
	void LoadAllSounds();

	/**
	 * 파일 경로로 WAV 데이터를 가져옵니다.
	 * @param FilePath 사운드 파일 경로 (Data/Sound/xxx.wav)
	 * @return FWavData 포인터 (nullptr if not found)
	 */
	FWavData* GetSound(const FString& FilePath);

	/**
	 * 모든 로드된 사운드 파일 경로 목록을 반환합니다.
	 * PropertyRenderer에서 ImGui 드롭다운을 만들 때 사용합니다.
	 */
	const TArray<FString>& GetAllSoundFilePaths() const { return SoundFilePaths; }

	/**
	 * XAudio2 인스턴스를 반환합니다.
	 * AudioComponent에서 SourceVoice 생성 시 필요합니다.
	 */
	IXAudio2* GetXAudio2() const { return XAudio2; }

	// --- 마스터 볼륨 제어 ---
	void SetMasterVolume(float Volume);
	float GetMasterVolume() const { return MasterVolume; }

	// --- 상태 확인 ---
	bool IsInitialized() const { return bIsInitialized; }

private:
	// --- 생성자 및 복사 방지 (싱글톤 패턴) ---
	UAudioManager();
	~UAudioManager();
	UAudioManager(const UAudioManager&) = delete;
	UAudioManager& operator=(const UAudioManager&) = delete;

	// --- 내부 헬퍼 함수 ---
	/**
	 * 특정 폴더를 재귀적으로 스캔하여 .wav 파일들을 찾습니다.
	 */
	void ScanFolderRecursive(const FWideString& FolderPath, TArray<FWideString>& OutWavFiles);

	/**
	 * .wav 파일을 로드하여 FWavData로 변환합니다.
	 */
	bool LoadWavFile(const FWideString& FilePath, FWavData* OutWavData);

private:
	// --- 멤버 변수 ---
	IXAudio2* XAudio2 = nullptr;								// XAudio2 인스턴스
	IXAudio2MasteringVoice* MasteringVoice = nullptr;			// 마스터링 보이스
	TMap<FString, std::unique_ptr<FWavData>> WavDataMap;		// 로드된 WAV 데이터 맵
	TArray<FString> SoundFilePaths;								// PropertyRenderer용 경로 목록
	float MasterVolume = 100.0f;								// 마스터 볼륨 (0 ~ 100)
	bool bIsInitialized = false;								// 초기화 여부
};
