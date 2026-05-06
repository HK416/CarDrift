# CarDrift

[English](#english) | [한국어](#한국어)

---

## English

A C++20 casual racing game project using Vulkan and ImGui.

### 🚀 Features
- **High Performance**: Built with C++20 and Vulkan for low-level graphics control.
- **Interactive UI**: Debugging and configuration via ImGui.
- **Math Library**: GLM for 3D mathematics.
- **Dependency Management**: vcpkg manifest mode for seamless builds.

### 🛠️ Tech Stack
- **Language**: C++20
- **Graphics API**: Vulkan
- **UI Framework**: ImGui
- **Mathematics**: GLM
- **Build System**: Visual Studio 2022 (MSBuild)
- **Package Manager**: vcpkg

### 🏗️ Getting Started

#### Prerequisites
- **Visual Studio 2022** (v143/v145 toolset)
- **Vulkan SDK**
- **vcpkg** (Integrated with Visual Studio)

#### Build Instructions
1. Clone the repository.
2. Open `CarDrift.slnx` in Visual Studio 2022.
3. Ensure vcpkg is integrated (`vcpkg integrate install`).
4. Select `x64` platform and `Debug` or `Release` configuration.
5. Build Solution (Ctrl+Shift+B). vcpkg will automatically restore dependencies.

---

## 한국어

Vulkan과 ImGui를 사용한 C++20 캐주얼 레이싱 게임 프로젝트입니다.

### 🚀 주요 기능
- **고성능**: 저수준 그래픽 제어를 위해 C++20과 Vulkan으로 제작되었습니다.
- **대화형 UI**: ImGui를 통한 디버깅 및 설정 기능 제공.
- **수학 라이브러리**: 3D 수학 연산을 위한 GLM 사용.
- **의존성 관리**: vcpkg 매니페스트 모드를 사용한 간편한 빌드 환경.

### 🛠️ 기술 스택
- **언어**: C++20
- **그래픽 API**: Vulkan
- **UI 프레임워크**: ImGui
- **수학**: GLM
- **빌드 시스템**: Visual Studio 2022 (MSBuild)
- **패키지 관리자**: vcpkg

### 🏗️ 시작하기

#### 사전 요구 사항
- **Visual Studio 2022** (v143/v145 툴셋)
- **Vulkan SDK**
- **vcpkg** (Visual Studio와 통합됨)

#### 빌드 방법
1. 저장소를 클론합니다.
2. Visual Studio 2022에서 `CarDrift.slnx`를 엽니다.
3. vcpkg가 통합되어 있는지 확인합니다 (`vcpkg integrate install`).
4. `x64` 플랫폼과 `Debug` 또는 `Release` 구성을 선택합니다.
5. 솔루션 빌드(Ctrl+Shift+B)를 수행합니다. vcpkg가 자동으로 의존성을 설치합니다.
