# 프로젝트 개요

이 프로젝트는 게임 모딩(Game Modding)의 필수적인 기술과 개념을 배우기 위한 교육용 리소스 저장소입니다. C++를 주요 언어로 사용하여 메모리 스캐닝, 후킹, 리버스 엔지니어링 등 다양한 모딩 기법을 다룹니다. 프로젝트는 여러 학습 모듈로 구성되어 있으며, 각 모듈은 특정 모딩 주제에 대한 실습을 제공합니다.

# 주요 기술 스택

*   **주요 언어:** C++
*   **보조 언어:** Python, TypeScript (일부 예제)
*   **플랫폼:** Windows
*   **그래픽스 API:** DirectX 11 (D3D11)
*   **UI 라이브러리:** ImGui (게임 내 오버레이용)
*   **빌드 시스템:** CMake

# 프로젝트 구조

*   `getting-started/`: 모딩 개발 환경 설정, 안전 가이드 등 입문자를 위한 문서가 있습니다. **가장 먼저 읽어보는 것을 권장합니다.**
*   `projects/`: 재사용 가능한 모딩 프레임워크, 게임 오버레이, 트레이너 등 핵심 프로젝트 예제가 있습니다.
*   `reference/`: 어셈블리, 언리얼 엔진 모딩 등 참고 자료를 제공합니다.
*   `resources/`: 코드 템플릿, 메모리 스캐너 등 개발에 유용한 리소스가 있습니다.
*   `scenario-***/`: 각 주제에 대한 학습 모듈입니다. (예: `scenario-pause-game`, `scenario-pak-modding`). 숫자 접두사가 제거되어 학습 순서를 유연하게 선택할 수 있습니다.

# 추천 학습 경로

이 프로젝트는 `getting-started/README.md`에 명시된 추천 학습 경로를 따르는 것을 권장합니다.

1.  **1단계 (도구 기반 입문):** `scenario-pak-modding`, `scenario-fromsoftware-modding` 등 도구를 활용하여 쉽고 빠르게 모딩을 경험합니다.
2.  **2단계 (원리 학습):** `scenario-pause-game`, `scenario-unlock-fps` 등 메모리 분석과 리버스 엔지니어링의 핵심 원리를 학습합니다.

# 주요 명령어

프로젝트의 C++ 코드는 `CMake`를 사용하여 빌드됩니다. 각 프로젝트 또는 학습 폴더의 `CMakeLists.txt` 파일을 참고하여 빌드할 수 있습니다.

일반적인 빌드 과정 예시:

```bash
mkdir build
cd build
cmake ..
make
```