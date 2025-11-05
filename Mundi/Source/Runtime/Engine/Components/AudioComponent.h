// Copyright Mundi Game Engine. All Rights Reserved.

#pragma once
#include "SceneComponent.h"
#include <xaudio2.h>
#include <x3daudio.h>

/**
 * UAudioComponent
 *
 * XAudio2 기반 3D 공간 오디오 재생 컴포넌트
 * - Data/Sound 폴더의 .wav 파일을 선택하여 재생
 * - 반복 재생, 볼륨/피치 제어 지원
 * - 3D 공간 오디오 지원 (위치 기반 사운드)
 * - Lua 스크립트에서 제어 가능
 */
class UAudioComponent : public USceneComponent
{
public:
	DECLARE_CLASS(UAudioComponent, USceneComponent)
	GENERATED_REFLECTION_BODY()
	DECLARE_DUPLICATE(UAudioComponent)

	UAudioComponent();

protected:
	~UAudioComponent() override;

public:
	// ─────────────── Lifecycle ───────────────
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;
	virtual void EndPlay(EEndPlayReason Reason) override;

	// ─────────────── Audio Control ───────────────
	/**
	 * 오디오를 재생합니다.
	 * @param bLoopOverride Loop 설정을 일시적으로 오버라이드하려면 true, 기본 bLoop 사용시 false
	 */
	void Play(bool bLoopOverride = false);

	/**
	 * 오디오 재생을 중지합니다.
	 * @param bImmediate 즉시 중지 여부
	 */
	void Stop(bool bImmediate = true);

	/**
	 * 재생 중인 오디오를 일시 정지합니다.
	 * @param bSavePosition 현재 재생 위치를 저장할지 여부 (기본값: true)
	 */
	void Pause(bool bSavePosition = true);

	/**
	 * 일시 정지된 오디오를 다시 재생합니다.
	 */
	void Resume();

	// ─────────────── Volume/Pitch Control ───────────────
	void SetVolume(float NewVolume);
	float GetVolume() const { return Volume; }

	void SetPlaybackSpeed(float NewSpeed);
	float GetPlaybackSpeed() const { return PlaybackSpeed; }

	void SetLoop(bool bNewLoop);
	bool IsLoop() const { return bLoop; }

	// ─────────────── State Query ───────────────
	/**
	 * 현재 재생 중인지 여부를 반환합니다.
	 */
	bool IsPlaying() const;

	/**
	 * 현재 재생 위치를 초 단위로 반환합니다.
	 */
	float GetPlaybackPosition() const;

	/**
	 * 재생 위치를 초 단위로 설정합니다.
	 */
	void SetPlaybackPosition(float Seconds);

	/**
	 * 오디오의 전체 길이를 초 단위로 반환합니다.
	 */
	float GetDuration() const;

	/**
	 * 재생 위치를 초 단위로 이동합니다 (현재 위치 + Seconds).
	 */
	void SeekRelative(float Seconds);

	// ─────────────── Audio File Management ───────────────
	/**
	 * 재생할 오디오 파일을 설정합니다.
	 * @param FilePath 오디오 파일 경로 (Data/Sound/xxx.wav)
	 */
	void SetAudioFile(const FString& FilePath);
	const FString& GetAudioFile() const { return AudioFilePath; }

	/**
	 * 컴포넌트 이름을 설정/반환합니다. (여러 AudioComponent 구분용)
	 */
	void SetComponentName(const FString& InName) { ComponentName = InName; }
	const FString& GetComponentName() const { return ComponentName; }

	/**
	 * 현재 설정된 오디오 파일을 로드합니다.
	 * BeginPlay에서 자동으로 호출되지만 수동으로도 호출 가능합니다.
	 */
	void LoadAudio();

	// ─────────────── Serialization ───────────────
	virtual void OnSerialized() override;
	virtual void DuplicateSubObjects() override;

private:
	// ─────────────── Internal Helpers ───────────────
	/**
	 * SourceVoice를 생성합니다.
	 */
	void CreateSourceVoice();

	/**
	 * SourceVoice를 해제합니다.
	 */
	void ReleaseSourceVoice();

private:
	FString ComponentName;			// 컴포넌트 이름 (구분용, 예: "Engine", "Booster", "Collision")
	FString AudioFilePath;			// 오디오 파일 경로
	float Volume = 100.0f;			// 볼륨 (0 ~ 100)
	float PlaybackSpeed = 1.0f;		// 재생 속도 (0.25 ~ 2.0배속, 피치도 함께 변경됨)
	bool bLoop = false;				// 반복 재생
	bool bAutoPlay = false;			// 자동 재생

	// ─────────────── Runtime Data (비직렬화) ───────────────
	IXAudio2SourceVoice* SourceVoice = nullptr;		// XAudio2 SourceVoice
	bool bIsLoaded = false;							// 오디오 파일 로드 여부
	bool bIsCurrentlyPlaying = false;				// 현재 재생 중인지 여부
	float PlaybackStartOffset = 0.0f;				// 재생 시작 오프셋 (초)
};
