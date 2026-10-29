# WP-P9D4 항아리 파괴 실행 지침

사람용 문서다. Claude context에 넣지 않는다. 상태는 예정 계획이며 코드·에셋·빌드·PIE는 미실행이다.

## 목표와 범위

항아리를 플레이어 근접 공격 한 번으로 파괴하고 서버 Loot 추첨으로 기존 DropItem을 생성한다.
첫 버전은 내구도 없는 단발 파괴·이동 통과이며 파편은 충돌/이동/피해에 영향을 주지 않는 로컬 표현이다.
Codex는 설계와 WP/CT를 작성하고 Claude Code는 앱에서 CT를 구현한다. 사람은 에셋과 PIE를 담당한다.

## 설계 근거

- 기준은 2026-10-29의 origin/master@2b59a69. 기존 ALB_DropItem과 ItemCode 기반 Inventory를 재사용한다.
- 기존 Hitbox는 Pawn만 감지하고 NormalAttack은 ASC/Health/Team을 요구하므로 두 연결점을 CT-002/003으로 분리한다.
- Loot는 Destroyed/EndPlay나 Chaos 파편 개수에 연결하지 않고 서버의 미파괴→파괴 전이에 연결한다.
- Geometry Collection은 BP 표현 이벤트에서 생성/재생한다. C++에 Chaos 모듈을 추가하지 않는다.
- 시스템 설계 원본은 LB-wiki의 world/destructible-prop.md, 작성 기준은 LB-wiki의 workflow/workflow-task-authoring.md다.

## 앱에서 실행하는 순서

| 순서 | CT | 다음 확인 |
| --- | --- | --- |
| 1 | CT-001 서버 파괴 상태 | E01에서 BP 표현·서버 파괴·지연 제거 |
| 2 | CT-002 Hitbox 감지 | 코드 검토 및 빌드 |
| 3 | CT-003 NormalAttack 연결 | E02 실제 공격과 생명체 피해 회귀 |
| 4 | CT-004 다중 Loot 데이터 | 추첨 횟수·중복 허용·항목 수량 계약 검토 |
| 5 | CT-005 DropItem 연결 | E03 Loot·획득, E99 최종 네트워크 확인 |

공통 계획을 포함한 master에서 feature/destructible-loot를 시작하고 WP-P9D4만 실행한다. 던전 WP의 Merge는 선행 조건이 아니다.
동시 작업 시 별도 체크아웃/에디터를 사용한다. 다른 WP의 코드 경로는 package.yaml protected와 각 CT allow로 분리한다.
계획 변경은 사용자가 검토한 뒤 Workflow Manager에서 별도로 Commit한다.
각 CT의 코드 검토·빌드·승인 Commit과 사람 에셋 작업은 기존 앱 절차를 따른다.
Content/Config/Build.cs/uproject 보호는 유지하고 에셋은 사람이 별도로 검토·저장한다.
WP의 모든 확인이 끝난 뒤 PR 검토·Merge를 수행하며 자동 Commit/Push/PR 생성은 하지 않는다.

## 이번 WP 확인 항목

첫 Claude 실행 전에 기준 커밋의 일반 LBEditor 빌드 결과를 기록한다. 아래 기본 공격 사전 확인까지 통과해야 항아리 연결을 시작한다. 현재 앱의 CT 이후 게이트가 이 사전 조건까지 자동 실행하는 것은 아니다.
새 테스트 맵은 /Game/LB/Tests/Destructible/Lvl_DestructibleTest이며 BP·Loot DA·Geometry Collection·필요한 테스트 전용 GameMode도 이 루트에 둔다.
기존 Lvl_GameplayTest·BP_GameMode·MasterItemTable·BP_DropItem·플레이어 BP·Config는 저장/수정하지 않고 참조한다. 필요한 외형/값은 이 루트의 테스트 파생 BP에서 설정한다.
항아리 최종 Mesh를 기다리지 않고 임시 큐브/기본 도형과 숨김 표현으로 상태·명중·Loot 검증을 진행할 수 있다. E99까지는 임시 도형 기반 Geometry Collection이라도 실제 Chaos 표현을 확인하며 최종 아트 교체는 요구하지 않는다.

구체적인 중간/최종 확인은 editor-guide.yaml에 있다.
시작 전 기존 플레이어 공격이 서버 OnMeleeHit까지 도달하는지 확인한다. 기존 콤보/시작 Section 실패는 항아리 구현 오류와 분리한다.
DropChance는 항아리당 한 번, Min/MaxRollCount는 추첨 횟수, Entry Min/MaxCount는 항목별 수량이다. 중복을 허용하고 합산하지 않으며 각 결과를 별도 Drop Actor로 분산 생성한다.
서버 실행 순서는 파괴 가드 확정·충돌 해제→Loot 생성 시도→BP 표현이다. Client에서 Drop Actor와 파괴 상태가 보이는 순서까지 동기화하는 계약은 아니다.
기존 BP_DropItem의 RefreshVisual은 C++에서 주석 처리돼 있다. BP 외형은 에디터 확인 필요다.
보이지 않는 드롭을 성공으로 판단하지 않도록 Actor 존재·ItemCode/Count·Inventory 수량도 확인한다.
드롭이 플레이어와 겹쳐 즉시 획득될 수 있으므로 드롭만 확인할 때는 플레이어를 분리한다.
이 상태에서 빌드 실패가 나타나면 새 CT와 기준 코드의 문제를 분리한다.
예를 들어 Inventory의 Tests/ToolMenusTestUtilities.h 포함은 기준 코드에 이미 존재하며 이번 WP의 수정 범위가 아니다.

## 후속 PIE와 제외 범위

몬스터의 항아리 공격, 다회 타격 내구도, 파편 물리 복제, 일반 피해 인터페이스,
보상 소유권/개인 Loot, Inventory/UI 개선, 저장/복구는 후속 작업이다.
던전 조각 안에 파괴 Actor를 넣는 통합은 WP-D6L2의 정적 조립 검증 이후 별도로 설계한다.
