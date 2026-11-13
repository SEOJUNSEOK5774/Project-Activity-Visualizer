# Project Activity Visualizer

[English](README.md) | [日本語](README.ja.md) | [한국어](README.ko.md)

에셋 저장 활동, Contributor 별칭, 현재 패키지 참조 관계, Hotspot 검토 후보, Git 이력을 확인하는 언리얼 에디터 플러그인입니다.

**버전:** 0.1.0  
**패키지 검증 환경:** Windows 64비트 / Unreal Engine 5.8.4

## 설치

- **빌드된 패키지:** [UE 5.8.4 / Win64용 0.1.0 ZIP 다운로드](https://github.com/SEOJUNSEOK5774/Project-Activity-Visualizer/releases/download/v0.1.0/ProjectActivityVisualizer_0.1.0_UE5.8.4_Win64.zip). 압축을 풀고 ProjectActivityVisualizer 폴더를 프로젝트의 Plugins 폴더에 복사합니다.
- **소스:** 이 저장소의 [ProjectActivityVisualizer](ProjectActivityVisualizer) 폴더를 프로젝트의 Plugins 폴더에 복사합니다. 필요하면 호환되는 언리얼 C++ 빌드 환경으로 빌드합니다. GitHub가 자동 생성하는 소스 ZIP에는 빌드된 바이너리가 포함되지 않습니다.
- Edit > Plugins에서 활성화한 뒤 Window > ProjectActivityVisualizer를 엽니다.

## 문서

- [설치·설정·지표 설명과 패키지 검증 결과](ProjectActivityVisualizer/README.ko.md)
- [예제 사용 안내](ProjectActivityVisualizer/EXAMPLE.ko.md)
- [0.1.0 변경 기록](ProjectActivityVisualizer/CHANGELOG.md)
- [배포 설명 초안과 ZIP 체크섬](docs/RELEASE_NOTES_0.1.0.md)
- [원래 기획안](docs/PROJECT_PLAN.md) — 보존한 설계 제안입니다. 현재 구현 동작은 최신 사용 문서를 참고합니다.

## 현재 구현 범위

저장 이벤트를 로컬 SQLite에 기록하고, 반복 저장을 고정된 UTC 5분 활동 구간으로 묶습니다. JSON 교환과 영구 캐시를 사용하는 Git 이력 조회를 지원합니다. Hotspot 후보에는 선정 조건을 표시하며, 활동 수치는 작업 시간이나 생산성을 나타내지 않습니다.

별도의 블루프린트 프로젝트에서 문서에 명시한 패키지 검증 항목을 통과했습니다. Git의 Invalid path '/Temp' 오류는 이 플러그인을 꺼도 재현되며 구체적인 원인과 해결 방법은 미확인 상태입니다. 전체 문서에서 검증 범위와 알려진 문제를 확인할 수 있습니다.

다음 개발 계획은 실시간 팀 웹 대시보드이며 이후 편집 감지를 진행합니다. 두 기능은 0.1.0에 포함되지 않습니다. 라이선스 선택은 원래 기획안에 기록된 대로 미정입니다.
