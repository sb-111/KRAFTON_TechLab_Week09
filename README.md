# Mundi Game Engine - Week 9 프로젝트

## 프로젝트 개요
3일간 진행된 Mundi 게임 엔진 개발 프로젝트로, 카메라 시스템, 오디오 시스템, 포스트 프로세싱, 그리고 스탠드얼론 빌드 기능을 구현했습니다.

## 주요 구현 기능

### 🎥 카메라 시스템
- **Player Camera Manager** (`PlayerCameraManager.cpp/h`)
  - 카메라 뷰 타겟 관리 및 페이드 인/아웃 효과
  - 포스트 프로세스 설정 통합 관리 (`FPostProcessSettings`)
  - 카메라 모디파이어 시스템 지원

- **Camera Modifier** (`CameraModifier.cpp/h`)
  - 카메라 Transform 수정 인터페이스
  - 포스트 프로세스 파라미터 동적 조정
  - 우선순위 기반 모디파이어 체인

- **Camera Shake Modifier** (`CameraShakeModifier.cpp/h`)
  - 카메라 흔들림 효과 구현
  - 진동 강도 및 패턴 제어

- **Spring Arm Component** (`SpringArmComponent.cpp/h`)
  - 3인칭 카메라를 위한 암 컴포넌트
  - 충돌 감지 및 자동 거리 조정
  - Lag 설정으로 부드러운 카메라 추적
  - 컨트롤러 회전 연동 지원

### 🎨 포스트 프로세싱
- **Vignetting Effect**
  - 화면 가장자리 어두운 효과 (Vignette Modifier)
  - 강도(Intensity), 반경(Radius), 색상 조정 가능
  - HLSL 셰이더 기반 실시간 렌더링

- **Letter Box Effect**
  - 시네마틱 레터박스(상하 검은 띠) 효과
  - 크기 및 색상 커스터마이징
  - 포스트 프로세스 체인 통합

- **Gamma Correction**
  - 감마 보정 기능 구현
  - 조정 가능한 감마 값 (기본 2.2)
  - 색상 정확도 개선

### 🔊 오디오 시스템
- **Audio Component** (`AudioComponent.cpp/h`)
  - XAudio2 기반 3D 공간 오디오
  - WAV 파일 재생 지원
  - Play/Stop/Pause/Resume 제어
  - 볼륨 및 피치 조정
  - 루프 재생 기능
  - 위치 기반 사운드 (3D Audio)

- **Audio Manager** (`AudioManager.cpp/h`)
  - 전역 오디오 시스템 관리
  - 리소스 로딩 및 캐싱

### 🚀 빌드 시스템
- **Standalone Build**
  - 독립 실행형 게임 빌드 기능 추가
  - Debug/Release 구성 지원
  - 실행 파일 패키징 (`Binaries/<Config>/Mundi.exe`)

## 기술 스택
- **언어**: C++20
- **그래픽 API**: Direct3D 11
- **오디오**: XAudio2 + X3DAudio
- **셰이더**: HLSL
- **빌드 도구**: MSBuild, Visual Studio

## 프로젝트 구조
```
Mundi/
├── Source/
│   ├── Runtime/
│   │   ├── Engine/
│   │   │   ├── Components/      # AudioComponent, CameraComponent, SpringArm 등
│   │   │   ├── GameFramework/   # PlayerCameraManager, CameraModifier 등
│   │   │   └── Audio/           # AudioManager
│   │   └── Renderer/            # SceneRenderer, PostProcess
│   ├── Editor/                  # 에디터 도구
│   └── Slate/                   # UI 위젯
├── Shaders/
│   └── PostProcess/             # 포스트 프로세스 셰이더 (Vignette, Letterbox 등)
├── Scene/                       # 씬 데이터
├── Data/                        # 게임 에셋
└── Binaries/                    # 빌드 결과물
```

## 빌드 방법
```bash
# Debug 빌드
msbuild Mundi.sln /p:Configuration=Debug /m

# Release 빌드
msbuild Mundi.sln /p:Configuration=Release /m

# 실행
Binaries/Debug/Mundi.exe
```

## 사용 예시

### 카메라 설정 (C++)
```cpp
// Spring Arm과 Camera Component를 사용한 3인칭 카메라
auto* SpringArm = CreateDefaultSubobject<USpringArmComponent>(L"SpringArm");
auto* Camera = CreateDefaultSubobject<UCameraComponent>(L"Camera");
Camera->AttachToComponent(SpringArm);

// Spring Arm 설정
SpringArm->TargetArmLength = 300.0f;
SpringArm->bEnableLag = true;
SpringArm->LagSpeed = 10.0f;
```

### 오디오 재생 (C++)
```cpp
// Audio Component 사용
auto* AudioComp = CreateDefaultSubobject<UAudioComponent>(L"BGM");
AudioComp->SetSound("background_music.wav");
AudioComp->SetVolume(0.8f);
AudioComp->Play(true); // 루프 재생
```

### 포스트 프로세스 설정 (C++)
```cpp
// Camera Manager에서 포스트 프로세스 설정
FPostProcessSettings PPSettings;
PPSettings.bEnableVignetting = true;
PPSettings.VignetteIntensity = 0.6f;
PPSettings.VignetteRadius = 0.7f;
PPSettings.bEnableLetterbox = true;
PPSettings.LetterboxSize = 0.1f;
PPSettings.Gamma = 2.2f;
```

## 팀원 기여
이번 프로젝트는 팀 협업으로 진행되었으며, 다음 기능들이 구현되었습니다:
- Camera Manager & Modifier 시스템
- Spring Arm Component
- Vignetting, Gamma Correction, Letter Box 포스트 프로세싱
- Audio Component 및 3D 오디오 시스템
- Standalone Build 기능

## 라이선스
Copyright Mundi Game Engine. All Rights Reserved.
