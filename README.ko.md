# Project Activity Visualizer

[English](README.md) | [日本語](README.ja.md) | [한국어](README.ko.md)

에셋 저장과 편집을 감지하고 Contributor 별칭, 패키지 참조 관계, Hotspot 검토 후보, Git 이력을 보여주는 언리얼 에디터 플러그인입니다. 로컬 웹 대시보드에서 편집과 저장을 하나의 타임라인으로 확인할 수 있습니다.

**버전** 0.2.0

**패키지 검증 환경** Windows x64 / Unreal Engine 5.8.3

## 설치

- [UE 5.8.3 / Win64용 0.2.0 ZIP 다운로드](https://github.com/SEOJUNSEOK5774/Project-Activity-Visualizer/releases/download/v0.2.0/ProjectActivityVisualizer-0.2.0-UE5.8.3-Win64.zip). 압축을 풀고 `ProjectActivityVisualizer` 폴더를 프로젝트의 `Plugins` 폴더에 복사합니다.
- **소스:** 이 저장소의 [ProjectActivityVisualizer](ProjectActivityVisualizer) 폴더를 프로젝트의 Plugins 폴더에 복사합니다. 필요하면 호환되는 언리얼 C++ 빌드 환경으로 빌드합니다. GitHub가 자동 생성하는 소스 ZIP에는 빌드된 바이너리가 포함되지 않습니다.
- Edit > Plugins에서 활성화한 뒤 Window > ProjectActivityVisualizer를 엽니다.
- Windows에 Python 3.9 이상을 설치하고 추적 대상 에셋을 저장합니다. 프로젝트의 `.uproject` 파일을 `WebDashboard/Start-Local.cmd`에 끌어다 놓으면 웹 대시보드가 열립니다.

## 문서

- [설치·설정·지표 설명과 패키지 검증 결과](ProjectActivityVisualizer/README.ko.md)
- [예제 사용 안내](ProjectActivityVisualizer/EXAMPLE.ko.md)
- [웹 대시보드 설정](WebDashboard/README.md)
- [변경 기록](ProjectActivityVisualizer/CHANGELOG.md)
- [0.2.0 릴리스 안내와 ZIP 체크섬](docs/RELEASE_NOTES_0.2.0.md)
- [원래 기획안](docs/PROJECT_PLAN.md) — 보존한 설계 제안입니다. 현재 구현 동작은 최신 사용 문서를 참고합니다.

## 현재 구현 범위

저장 이벤트를 로컬 SQLite에 기록하고, 반복 저장을 고정된 UTC 5분 활동 구간으로 묶습니다. JSON 교환과 영구 캐시를 사용하는 Git 이력 조회를 지원합니다. Hotspot 후보에는 선정 조건을 표시하며, 활동 수치는 작업 시간이나 생산성을 나타내지 않습니다.

별도의 블루프린트 프로젝트에서 문서에 명시한 패키지 검증 항목을 통과했습니다. Git의 Invalid path '/Temp' 오류는 이 플러그인을 꺼도 재현되며 구체적인 원인과 해결 방법은 미확인 상태입니다. 전체 문서에서 검증 범위와 알려진 문제를 확인할 수 있습니다.

웹 대시보드는 에셋이 처음 미저장 상태가 된 시각과 이후 저장 시각을 각각 표시합니다. Python 시작 스크립트를 설정하면 직접 참조하는 에셋을 그래프에서 확인하고 언리얼 에디터에서 열 수 있습니다. 라이선스 선택은 원래 기획안에 기록된 대로 미정입니다.
