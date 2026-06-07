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
- **Modern Architecture**: Well-structured source code separated into core, graphics, scene, and utility modules.
- **Automated Shader Compilation**: Visual Studio automatically compiles GLSL shaders into SPIR-V using `glslc`.

### 🛠️ Tech Stack
- **Language**: C++20
- **Graphics API**: Vulkan (Dynamic Rendering, Synchronization2)
- **UI Framework**: ImGui
- **Mathematics**: GLM
- **Build System**: Visual Studio 2022 (MSBuild)
- **Package Manager**: vcpkg (Manifest Mode)

### 🏗️ Getting Started

#### Prerequisites
- **Visual Studio 2022** (v143/v145 toolset)
- **Vulkan SDK** (Ensure `VULKAN_SDK` environment variable is set and `glslc` is in your PATH)
- **vcpkg** (Integrated with Visual Studio)

#### Build Instructions
1. Clone the repository.
2. Open `CarDrift.slnx` in Visual Studio 2022.
3. Ensure vcpkg is integrated (`vcpkg integrate install`).
4. Select `x64` platform and `Debug` or `Release` configuration.
5. Build Solution (Ctrl+Shift+B).
   - vcpkg will automatically restore dependencies.
   - MSBuild will automatically compile `.vert` and `.frag` shaders using the Vulkan SDK.

### 📁 Project Structure
- `src/core/`: Core systems, input management, and application entry point.
- `src/graphics/`: Vulkan renderer, materials, meshes, and lighting.
- `src/scene/`: Scene management, game objects, and cameras.
- `src/utils/`: Utilities like GLTF loading.
- `shaders/`: GLSL shader source files.

---

## 한국어

Vulkan과 ImGui를 사용한 C++20 캐주얼 레이싱 게임 프로젝트입니다.

### 🚀 주요 기능
- **고성능**: 저수준 그래픽 제어를 위해 C++20과 Vulkan으로 제작되었습니다.
- **대화형 UI**: ImGui를 통한 디버깅 및 설정 기능 제공.
- **수학 라이브러리**: 3D 수학 연산을 위한 GLM 사용.
- **의존성 관리**: vcpkg 매니페스트 모드를 사용한 간편한 빌드 환경.
- **모던 아키텍처**: core, graphics, scene, utils 모듈로 체계적으로 분리된 소스 코드.
- **자동 셰이더 컴파일**: Visual Studio 빌드 시 `glslc`를 통해 GLSL 셰이더가 자동으로 SPIR-V로 컴파일됩니다.

### 🛠️ 기술 스택
- **언어**: C++20
- **그래픽 API**: Vulkan (Dynamic Rendering, Synchronization2)
- **UI 프레임워크**: ImGui
- **수학**: GLM
- **빌드 시스템**: Visual Studio 2022 (MSBuild)
- **패키지 관리자**: vcpkg (매니페스트 모드)

### 🏗️ 시작하기

#### 사전 요구 사항
- **Visual Studio 2022** (v143/v145 툴셋)
- **Vulkan SDK** (`VULKAN_SDK` 환경 변수가 설정되어 있고, `glslc`가 PATH에 있어야 합니다)
- **vcpkg** (Visual Studio와 통합됨)

#### 빌드 방법
1. 저장소를 클론합니다.
2. Visual Studio 2022에서 `CarDrift.slnx`를 엽니다.
3. vcpkg가 통합되어 있는지 확인합니다 (`vcpkg integrate install`).
4. `x64` 플랫폼과 `Debug` 또는 `Release` 구성을 선택합니다.
5. 솔루션 빌드(Ctrl+Shift+B)를 수행합니다.
   - vcpkg가 자동으로 의존성을 설치합니다.
   - MSBuild가 Vulkan SDK를 사용하여 `.vert` 및 `.frag` 셰이더를 자동으로 컴파일합니다.

### 📁 프로젝트 구조
- `src/core/`: 핵심 시스템, 입력 관리, 애플리케이션 진입점.
- `src/graphics/`: Vulkan 렌더러, 머티리얼, 메쉬, 라이팅.
- `src/scene/`: 씬 관리, 게임 오브젝트, 카메라.
- `src/utils/`: GLTF 로딩 등 유틸리티.
- `shaders/`: GLSL 셰이더 소스 파일.
