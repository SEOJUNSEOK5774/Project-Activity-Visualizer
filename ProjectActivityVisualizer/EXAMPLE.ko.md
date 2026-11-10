# Project Activity Visualizer: 예제 사용 안내

[English](EXAMPLE.md) | [日本語](EXAMPLE.ja.md) | [한국어](EXAMPLE.ko.md)

[설치와 지표 설명](README.ko.md) | [0.1.0 변경 기록](CHANGELOG.md)

이 안내는 사용자의 프로젝트 안에 작은 예제를 직접 만드는 절차입니다. 예제 .uproject, .uasset, 활동 DB, Git 저장소를 함께 제공하는 파일은 아닙니다.

패키지 기본 동작을 확인한 환경은 Windows 64비트 / Unreal Engine 5.8.4입니다. 기존 프로젝트에 활동 기록이나 별도 수집 규칙이 있다면 별도의 테스트 프로젝트에서 따라 하는 편이 결과를 확인하기 쉽습니다.

## 1. 프로젝트 준비

1. README.ko.md의 설치 절차에 따라 플러그인을 설치하고 Edit > Plugins에서 활성화합니다.
2. Editor Preferences > Plugins > Project Activity에서 Contributor Alias를 demo_a로 설정합니다. 예제에서는 Enable Automatic Import를 끈 상태로 둡니다.
3. Project Settings > Plugins > Project Activity에서 활동 수집을 켭니다. Tracked Paths에 /Game/ActivityExample/을 포함하고 Ignored Paths가 이 폴더를 제외하지 않는지 확인합니다.
4. Hotspot 예제도 확인하려면 기본 기준을 유지합니다. Minimum Activity Buckets = 3, Minimum Named Aliases = 2, Minimum Referencers = 1입니다.

## 2. 테스트 에셋 만들고 저장하기

1. 콘텐츠 브라우저에서 Content 바로 아래에 ActivityExample 폴더를 만듭니다.
2. 그 폴더에 Actor 블루프린트를 만들고 이름을 BP_ActivityExample로 지정합니다.
3. Integer 변수 TestValue를 추가합니다.
4. 블루프린트를 Compile한 뒤 TestValue 기본값을 1로 설정하고 다시 Compile하여 저장합니다.
5. Window > ProjectActivityVisualizer를 열고 All time을 선택한 다음 Refresh를 누릅니다.

Activity 페이지에서 /Game/ActivityExample/BP_ActivityExample을 확인합니다. 이 에셋에 Raw saves와 활동 구간이 각각 1개 이상 기록되어야 합니다. 에셋을 만드는 동안 저장한 횟수에 따라 초기 수치는 달라질 수 있습니다.

## 3. 저장 횟수와 활동 구간 비교하기

TestValue를 2로 변경하고 Compile하여 저장합니다. 이어서 3으로 변경하고 다시 Compile하여 저장한 다음 대시보드의 Refresh를 누릅니다.

수집기가 저장 이벤트를 기록하면 Raw saves가 증가합니다. 동일 패키지·동일 별칭의 저장은 고정된 UTC 5분 구간 안에서 하나의 활동 구간으로 묶입니다. 구간 경계를 넘으면 이전 저장으로부터 5분이 지나지 않았어도 새 활동 구간이 추가됩니다. 변경 없는 에셋을 저장하면 새 이벤트가 발생하지 않을 수 있으므로 매번 실제 값을 바꾼 뒤 저장합니다.

일별 그래프는 UTC 날짜를 사용합니다. 카테고리 규칙이 없는 새 프로젝트라면 보통 Other에 집계됩니다.

## 4. 별칭과 Hotspot 후보 확인하기

1. Contributor Alias를 demo_b로 바꿉니다. TestValue를 4로 바꾸고 Compile하여 저장한 뒤 대시보드를 갱신합니다.
2. Collaboration / Hotspot을 엽니다. 여러 별칭이 기록된 패키지 목록에서 이 에셋을 찾고 Show Aliases로 demo_a와 demo_b를 확인합니다.
3. Contributor Alias를 demo_c로 바꿉니다. TestValue를 5로 바꾸고 Compile하여 저장한 뒤 다시 갱신합니다.

활동 구간을 구분하는 기준에 별칭이 포함되므로 이제 활동 구간은 최소 3개, 이름 있는 별칭은 3종류가 됩니다. 기본 Hotspot 기준에서는 활동 구간 상위 10개 표본에 포함될 경우 여러 별칭 조건을 만족합니다. 새 예제 프로젝트에서는 이 목록에서 쉽게 찾을 수 있습니다.

이는 한 사람이 테스트용 이름을 바꿔 가며 만든 기록입니다. 실제 기여자 3명이나 동시 편집을 의미하지 않습니다. 현재 별칭을 바꿔도 과거 이벤트의 별칭은 변경되지 않습니다. 예제가 끝나면 사용할 별칭으로 되돌립니다.

## 5. JSON 교환 확인하기

1. Export all JSON을 누르고 프로젝트의 Saved/ProjectActivity/Exports 폴더에서 내보낸 파일을 찾습니다.
2. 같은 프로젝트에서 Import JSON으로 그 파일을 가져옵니다.
3. 이미 존재하는 이벤트 ID이므로 해당 기록을 건너뛰며 전체 기록 수가 증가하지 않아야 합니다.

활동을 교환할 때는 프로젝트 이름이 같은 데이터끼리 사용합니다. 활동 JSON에는 저장 기록이 포함되며 Git 이력 캐시는 포함되지 않습니다. 실행 중인 에디터 사이에서 작업 중인 DB를 직접 복사하지 않습니다.

## 6. 선택 사항: Git 이력과 캐시

Git 설치, 프로젝트가 들어 있는 로컬 저장소, 언리얼 에디터의 Git Provider 연결, BP_ActivityExample.uasset을 포함하는 커밋이 하나 이상 필요합니다. Git 설정은 활동 수집과 별도로 준비합니다.

1. Activity 페이지에서 BP_ActivityExample 옆의 Show History를 누릅니다.
2. Commit history에서 패키지 상태와 커밋 작성자·메시지를 확인합니다. 조회에 성공하면 Local copy updated가 표시됩니다.
3. 에디터를 재시작하고 Refresh나 Show History를 누르기 전에 Commit history를 엽니다. 기존 조회 시각을 유지한 CACHED RESULT가 표시되어야 합니다.
4. 캐시 대체 표시를 확인하려면 Revision Control Provider를 잠시 None으로 바꾸고 플러그인의 Refresh를 누릅니다. 사용 불가 안내와 함께 저장된 결과가 표시되어야 합니다.
5. Git을 다시 연결하고 갱신합니다. 조회에 성공하면 로컬 복사본과 조회 시각이 갱신됩니다.

Git 작성자와 Contributor Alias는 별개의 정보입니다. 알려진 Invalid path '/Temp' 소스 컨트롤 로그 문제는 README.ko.md에 기록되어 있으며, 이 플러그인을 꺼도 재현됩니다.

## 이 예제에서 확인하는 내용

- 저장 이벤트, 고정된 5분 활동 구간, 대시보드 갱신
- 기록에 남은 별칭과 설정 가능한 Hotspot 조건 하나
- JSON 가져오기의 중복 방지
- 선택적으로 Git 이력, 재시작 캐시 복원, Provider 비활성화 시 캐시 표시

실제 작업 시간을 측정하거나 저장하지 않은 편집을 감지하는 예제는 아닙니다. 실시간 웹 대시보드는 제공하지 않습니다.
