// Copyright Mundi Game Engine. All Rights Reserved.

#include "pch.h"
#include "AudioComponent.h"
#include "AudioManager.h"
#include "ObjectFactory.h"

IMPLEMENT_CLASS(UAudioComponent)

BEGIN_PROPERTIES(UAudioComponent)
	MARK_AS_COMPONENT("Audio Component", "XAudio2 기반 오디오 재생 컴포넌트")
	ADD_PROPERTY_AUDIO(FString, AudioFilePath, "Audio", true, "재생할 오디오 파일 경로 (Data/Sound/xxx.wav)")
	ADD_PROPERTY_RANGE(float, Volume, "Audio", 0.0f, 100.0f, true, "볼륨 (0 ~ 100)")
	ADD_PROPERTY_RANGE(float, PlaybackSpeed, "Audio", 0.25f, 2.0f, true, "재생 속도 (0.25 ~ 2.0배속, 피치도 함께 변경됨)")
	ADD_PROPERTY(bool, bLoop, "Audio", true, "반복 재생 여부")
	ADD_PROPERTY(bool, bAutoPlay, "Audio", true, "BeginPlay 시 자동 재생 여부")
END_PROPERTIES()

UAudioComponent::UAudioComponent()
	: AudioFilePath("")
	, Volume(100.0f)
	, PlaybackSpeed(1.0f)
	, bLoop(false)
	, bAutoPlay(false)
	, SourceVoice(nullptr)
	, bIsLoaded(false)
{
	// AudioComponent는 기본적으로 Tick 불필요
	bCanEverTick = false;
	bTickEnabled = false;
}

UAudioComponent::~UAudioComponent()
{
	ReleaseSourceVoice();
}

void UAudioComponent::InitializeComponent()
{
	Super_t::InitializeComponent();

	// AudioManager 초기화 확인
	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
	{
		UE_LOG("AudioManager not initialized. AudioComponent will not function properly.");
	}
}

void UAudioComponent::BeginPlay()
{
	Super_t::BeginPlay();

	// 오디오 파일 로드
	if (!AudioFilePath.empty())
	{
		LoadAudio();
	}

	// 자동 재생
	if (bAutoPlay && bIsLoaded)
	{
		Play(bLoop);
	}
}

void UAudioComponent::TickComponent(float DeltaTime)
{
	Super_t::TickComponent(DeltaTime);

	// 필요시 상태 업데이트 로직 추가 가능
}

void UAudioComponent::EndPlay(EEndPlayReason Reason)
{
	// 재생 중지 및 리소스 해제
	Stop(true);
	ReleaseSourceVoice();

	Super_t::EndPlay(Reason);
}

void UAudioComponent::Play(bool bLoopOverride)
{
	if (!bIsLoaded || !SourceVoice)
	{
		UE_LOG("AudioComponent::Play - Audio not loaded or SourceVoice is null.");
		return;
	}

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	FWavData* WavData = AudioMgr.GetSound(AudioFilePath);
	if (!WavData)
	{
		UE_LOG("AudioComponent::Play - Failed to get WAV data.");
		return;
	}

	// 루프 설정
	bool bShouldLoop = bLoopOverride ? true : bLoop;

	// 버퍼 설정
	XAUDIO2_BUFFER Buffer = {};
	Buffer.AudioBytes = WavData->AudioDataSize;
	Buffer.pAudioData = WavData->AudioData;
	Buffer.Flags = XAUDIO2_END_OF_STREAM;

	if (bShouldLoop)
	{
		Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
	else
	{
		Buffer.LoopCount = 0;
	}

	// 기존 버퍼 클리어
	SourceVoice->FlushSourceBuffers();

	// 버퍼 제출
	HRESULT hr = SourceVoice->SubmitSourceBuffer(&Buffer);
	if (FAILED(hr))
	{
		UE_LOG("AudioComponent::Play - Failed to submit source buffer: 0x%08X", hr);
		return;
	}

	// 재생 시작
	hr = SourceVoice->Start(0);
	if (FAILED(hr))
	{
		UE_LOG("AudioComponent::Play - Failed to start playback: 0x%08X", hr);
		return;
	}

	bIsCurrentlyPlaying = true;
	PlaybackStartOffset = 0.0f;  // 처음부터 재생
	UE_LOG("AudioComponent::Play - '%s' started. Loop: %s", AudioFilePath.c_str(), bShouldLoop ? "true" : "false");
}

void UAudioComponent::Stop(bool bImmediate)
{
	if (!SourceVoice)
		return;

	// AudioManager가 이미 shutdown된 경우 XAudio2 리소스가 무효화되었을 수 있음
	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
	{
		// AudioManager가 종료되었으면 SourceVoice도 무효화됨
		SourceVoice = nullptr;
		return;
	}

	try
	{
		if (bImmediate)
		{
			SourceVoice->Stop(0);
			SourceVoice->FlushSourceBuffers();
		}
		else
		{
			SourceVoice->Stop(XAUDIO2_PLAY_TAILS);
		}

		// 재생 상태를 먼저 false로 설정 (SetPlaybackPosition이 재생을 시작하지 않도록)
		bIsCurrentlyPlaying = false;
		PlaybackStartOffset = 0.0f;

		// 재생 위치를 0으로 리셋
		if (bImmediate)
		{
			SetPlaybackPosition(0.0f);
		}

		UE_LOG("AudioComponent::Stop - '%s' stopped.", AudioFilePath.c_str());
	}
	catch (...)
	{
		// XAudio2 호출 실패 시 예외 처리
		UE_LOG("AudioComponent::Stop - Exception occurred while stopping audio.");
		SourceVoice = nullptr;
		bIsCurrentlyPlaying = false;
		PlaybackStartOffset = 0.0f;
	}
}

void UAudioComponent::Pause(bool bSavePosition)
{
	if (!SourceVoice)
		return;

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
	{
		SourceVoice = nullptr;
		return;
	}

	// bSavePosition이 true일 때만 현재 재생 위치를 PlaybackStartOffset에 저장
	if (bSavePosition && bIsCurrentlyPlaying)
	{
		FWavData* WavData = AudioMgr.GetSound(AudioFilePath);
		if (WavData)
		{
			try
			{
				XAUDIO2_VOICE_STATE State;
				SourceVoice->GetState(&State);
				float CurrentSeconds = (float)State.SamplesPlayed / (float)WavData->WaveFormat.nSamplesPerSec;
				PlaybackStartOffset = PlaybackStartOffset + CurrentSeconds;
			}
			catch (...) {}
		}
	}

	try
	{
		SourceVoice->Stop(0);
		SourceVoice->FlushSourceBuffers();
		bIsCurrentlyPlaying = false;
		UE_LOG("AudioComponent::Pause - '%s' paused at %.2f seconds.", AudioFilePath.c_str(), PlaybackStartOffset);
	}
	catch (...)
	{
		UE_LOG("AudioComponent::Pause - Exception occurred.");
		SourceVoice = nullptr;
		bIsCurrentlyPlaying = false;
	}
}

void UAudioComponent::Resume()
{
	if (!bIsLoaded)
		return;

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
	{
		SourceVoice = nullptr;
		return;
	}

	FWavData* WavData = AudioMgr.GetSound(AudioFilePath);
	if (!WavData)
		return;

	try
	{
		// SourceVoice를 재생성 (SamplesPlayed 누적 문제 해결)
		if (SourceVoice)
		{
			SourceVoice->Stop(0);
			SourceVoice->FlushSourceBuffers();
			SourceVoice->DestroyVoice();
			SourceVoice = nullptr;
		}

		// 새 SourceVoice 생성
		IXAudio2* XAudio2 = AudioMgr.GetXAudio2();
		if (!XAudio2)
			return;

		HRESULT hr = XAudio2->CreateSourceVoice(&SourceVoice, &WavData->WaveFormat);
		if (FAILED(hr))
		{
			UE_LOG("AudioComponent::Resume - Failed to create SourceVoice: 0x%08X", hr);
			return;
		}

		// 볼륨/재생속도 설정
		float NormalizedVolume = Volume / 100.0f;
		SourceVoice->SetVolume(NormalizedVolume);
		SourceVoice->SetFrequencyRatio(PlaybackSpeed);

		// 현재 PlaybackStartOffset 위치에서 버퍼 제출
		UINT64 SamplePosition = (UINT64)(PlaybackStartOffset * WavData->WaveFormat.nSamplesPerSec);
		UINT64 TotalSamples = WavData->AudioDataSize / WavData->WaveFormat.nBlockAlign;

		if (SamplePosition >= TotalSamples)
			SamplePosition = TotalSamples - 1;

		DWORD ByteOffset = (DWORD)(SamplePosition * WavData->WaveFormat.nBlockAlign);

		// 버퍼 제출
		XAUDIO2_BUFFER Buffer = {};
		Buffer.AudioBytes = WavData->AudioDataSize - ByteOffset;
		Buffer.pAudioData = WavData->AudioData + ByteOffset;
		Buffer.Flags = XAUDIO2_END_OF_STREAM;

		if (bLoop)
		{
			Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		}
		else
		{
			Buffer.LoopCount = 0;
		}

		SourceVoice->SubmitSourceBuffer(&Buffer);

		// 재생 시작
		SourceVoice->Start(0);
		bIsCurrentlyPlaying = true;
		UE_LOG("AudioComponent::Resume - '%s' resumed from %.2f seconds.", AudioFilePath.c_str(), PlaybackStartOffset);
	}
	catch (...)
	{
		UE_LOG("AudioComponent::Resume - Exception occurred.");
		SourceVoice = nullptr;
		bIsCurrentlyPlaying = false;
	}
}

void UAudioComponent::SetVolume(float NewVolume)
{
	Volume = std::clamp(NewVolume, 0.0f, 100.0f);

	if (!SourceVoice)
		return;

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
	{
		SourceVoice = nullptr;
		return;
	}

	try
	{
		// XAudio2는 0.0 ~ 1.0 범위를 사용하므로 변환
		float NormalizedVolume = Volume / 100.0f;
		SourceVoice->SetVolume(NormalizedVolume);
	}
	catch (...)
	{
		SourceVoice = nullptr;
	}
}

void UAudioComponent::SetPlaybackSpeed(float NewSpeed)
{
	PlaybackSpeed = std::clamp(NewSpeed, 0.25f, 2.0f);

	if (!SourceVoice)
		return;

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
	{
		SourceVoice = nullptr;
		return;
	}

	try
	{
		SourceVoice->SetFrequencyRatio(PlaybackSpeed);
	}
	catch (...)
	{
		SourceVoice = nullptr;
	}
}

void UAudioComponent::SetLoop(bool bNewLoop)
{
	bLoop = bNewLoop;

	// 이미 재생 중이면 다시 재생해야 루프 설정 적용
	if (IsPlaying())
	{
		Stop(true);
		Play(bLoop);
	}
}

bool UAudioComponent::IsPlaying() const
{
	return bIsCurrentlyPlaying;
}

float UAudioComponent::GetPlaybackPosition() const
{
	if (!SourceVoice || !bIsLoaded)
		return 0.0f;

	// 재생 중이 아니면 PlaybackStartOffset만 반환 (멈춘 위치)
	if (!bIsCurrentlyPlaying)
		return PlaybackStartOffset;

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
		return 0.0f;

	FWavData* WavData = AudioMgr.GetSound(AudioFilePath);
	if (!WavData)
		return 0.0f;

	try
	{
		XAUDIO2_VOICE_STATE State;
		SourceVoice->GetState(&State);

		// SamplesPlayed를 초 단위로 변환하고 시작 오프셋 추가
		float Seconds = (float)State.SamplesPlayed / (float)WavData->WaveFormat.nSamplesPerSec;
		return PlaybackStartOffset + Seconds;
	}
	catch (...)
	{
		return 0.0f;
	}
}

void UAudioComponent::SetPlaybackPosition(float Seconds)
{
	if (!bIsLoaded)
		return;

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
	{
		SourceVoice = nullptr;
		return;
	}

	FWavData* WavData = AudioMgr.GetSound(AudioFilePath);
	if (!WavData)
		return;

	try
	{
		// 재생 중이었는지 확인
		bool bWasPlaying = IsPlaying();

		// SourceVoice를 재생성 (SamplesPlayed 누적 문제 해결)
		if (SourceVoice)
		{
			SourceVoice->Stop(0);
			SourceVoice->FlushSourceBuffers();
			SourceVoice->DestroyVoice();
			SourceVoice = nullptr;
		}

		// 새 SourceVoice 생성
		IXAudio2* XAudio2 = AudioMgr.GetXAudio2();
		if (!XAudio2)
			return;

		HRESULT hr = XAudio2->CreateSourceVoice(&SourceVoice, &WavData->WaveFormat);
		if (FAILED(hr))
		{
			UE_LOG("AudioComponent::SetPlaybackPosition - Failed to create SourceVoice: 0x%08X", hr);
			return;
		}

		// 볼륨/재생속도 설정
		float NormalizedVolume = Volume / 100.0f;
		SourceVoice->SetVolume(NormalizedVolume);
		SourceVoice->SetFrequencyRatio(PlaybackSpeed);

		// 초를 샘플 수로 변환
		UINT64 SamplePosition = (UINT64)(Seconds * WavData->WaveFormat.nSamplesPerSec);
		UINT64 TotalSamples = WavData->AudioDataSize / WavData->WaveFormat.nBlockAlign;

		// 범위 체크
		if (SamplePosition >= TotalSamples)
			SamplePosition = TotalSamples - 1;

		// 바이트 오프셋 계산
		DWORD ByteOffset = (DWORD)(SamplePosition * WavData->WaveFormat.nBlockAlign);

		// 새 위치에서 버퍼 제출
		XAUDIO2_BUFFER Buffer = {};
		Buffer.AudioBytes = WavData->AudioDataSize - ByteOffset;
		Buffer.pAudioData = WavData->AudioData + ByteOffset;
		Buffer.Flags = XAUDIO2_END_OF_STREAM;

		if (bLoop)
		{
			Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		}
		else
		{
			Buffer.LoopCount = 0;
		}

		SourceVoice->SubmitSourceBuffer(&Buffer);

		// 시작 오프셋 업데이트 (새로운 버퍼는 이 위치부터 시작)
		PlaybackStartOffset = Seconds;

		// 재생 중이었으면 다시 재생
		if (bWasPlaying)
		{
			SourceVoice->Start(0);
			bIsCurrentlyPlaying = true;
		}
		else
		{
			bIsCurrentlyPlaying = false;
		}
	}
	catch (...)
	{
		UE_LOG("AudioComponent::SetPlaybackPosition - Exception occurred.");
		SourceVoice = nullptr;
		bIsCurrentlyPlaying = false;
	}
}

float UAudioComponent::GetDuration() const
{
	if (!bIsLoaded)
		return 0.0f;

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
		return 0.0f;

	FWavData* WavData = AudioMgr.GetSound(AudioFilePath);
	if (!WavData)
		return 0.0f;

	// 전체 샘플 수 / 샘플레이트 = 초
	UINT64 TotalSamples = WavData->AudioDataSize / WavData->WaveFormat.nBlockAlign;
	return (float)TotalSamples / (float)WavData->WaveFormat.nSamplesPerSec;
}

void UAudioComponent::SeekRelative(float Seconds)
{
	float CurrentPos = GetPlaybackPosition();
	float Duration = GetDuration();
	float NewPos = CurrentPos + Seconds;

	// 범위 제한: 0 ~ Duration
	NewPos = std::clamp(NewPos, 0.0f, Duration);

	SetPlaybackPosition(NewPos);
}

void UAudioComponent::SetAudioFile(const FString& FilePath)
{
	// 기존 사운드 중지 및 해제
	Stop(true);
	ReleaseSourceVoice();

	// 새 경로 설정
	AudioFilePath = FilePath;
	bIsLoaded = false;

	// 새 오디오 로드
	if (!AudioFilePath.empty())
	{
		LoadAudio();
	}
}

void UAudioComponent::LoadAudio()
{
	if (AudioFilePath.empty())
	{
		UE_LOG("AudioComponent::LoadAudio - AudioFilePath is empty.");
		return;
	}

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (!AudioMgr.IsInitialized())
	{
		UE_LOG("AudioComponent::LoadAudio - AudioManager not initialized.");
		return;
	}

	// WAV 데이터 가져오기
	FWavData* WavData = AudioMgr.GetSound(AudioFilePath);
	if (!WavData)
	{
		UE_LOG("AudioComponent::LoadAudio - Sound file not found: '%s'", AudioFilePath.c_str());
		bIsLoaded = false;
		return;
	}

	// SourceVoice 생성
	CreateSourceVoice();

	if (SourceVoice)
	{
		bIsLoaded = true;
		UE_LOG("AudioComponent::LoadAudio - Successfully loaded '%s'", AudioFilePath.c_str());
	}
	else
	{
		bIsLoaded = false;
	}
}

void UAudioComponent::CreateSourceVoice()
{
	// 기존 SourceVoice 해제
	ReleaseSourceVoice();

	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	IXAudio2* XAudio2 = AudioMgr.GetXAudio2();
	if (!XAudio2)
	{
		UE_LOG("AudioComponent::CreateSourceVoice - XAudio2 not available.");
		return;
	}

	FWavData* WavData = AudioMgr.GetSound(AudioFilePath);
	if (!WavData)
	{
		UE_LOG("AudioComponent::CreateSourceVoice - WAV data not found.");
		return;
	}

	// SourceVoice 생성
	HRESULT hr = XAudio2->CreateSourceVoice(&SourceVoice, &WavData->WaveFormat);
	if (FAILED(hr))
	{
		UE_LOG("AudioComponent::CreateSourceVoice - Failed to create SourceVoice: 0x%08X", hr);
		SourceVoice = nullptr;
		return;
	}

	// 초기 볼륨/재생속도 설정
	float NormalizedVolume = Volume / 100.0f;
	SourceVoice->SetVolume(NormalizedVolume);
	SourceVoice->SetFrequencyRatio(PlaybackSpeed);
}

void UAudioComponent::ReleaseSourceVoice()
{
	if (!SourceVoice)
		return;

	// 먼저 재생 중지
	Stop(true);

	// AudioManager가 shutdown된 경우 DestroyVoice 호출 불필요
	UAudioManager& AudioMgr = UAudioManager::GetInstance();
	if (AudioMgr.IsInitialized())
	{
		try
		{
			SourceVoice->DestroyVoice();
		}
		catch (...)
		{
			// DestroyVoice 실패 시 예외 처리
			UE_LOG("AudioComponent::ReleaseSourceVoice - Exception occurred while destroying voice.");
		}
	}

	SourceVoice = nullptr;
	bIsLoaded = false;
}

void UAudioComponent::OnSerialized()
{
	Super_t::OnSerialized();

	// 직렬화 후 오디오 로드는 하지 않음
	// BeginPlay에서 자동으로 로드될 것임
	// (복제 시점에 AudioManager 상태가 불확실할 수 있음)
}

void UAudioComponent::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();

	// 이 함수는 '복사본' (PIE 컴포넌트)에서 실행됩니다.
	// SourceVoice는 원본의 포인터를 얕은 복사한 상태이므로,
	// ReleaseSourceVoice()를 호출하면 원본의 SourceVoice를 파괴하게 됩니다.
	// 따라서 단순히 nullptr로 설정만 하고, BeginPlay에서 새로 생성합니다.
	SourceVoice = nullptr;
	bIsLoaded = false;
	bIsCurrentlyPlaying = false;
	PlaybackStartOffset = 0.0f;
}
