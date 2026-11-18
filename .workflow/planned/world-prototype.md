# 월드 오브젝트·던전 방 전환 예정 작업

> 상태: 예정 계획. 2026-10-29 origin/master@2b59a69 기준. 코드·에셋·빌드·PIE 미실행.

## 보관과 시작

사람용 대기열 문서다. Claude context에는 넣지 않는다.
Codex가 설계와 WP/CT를 작성하고 Claude Code가 코드 구현을 진행한다.
사람은 에셋 연결·PIE·Diff 확인과 Commit/Push/PR 승인을 담당한다.
실제 WP는 `.workflow/changes/`에 두고 실행 계약은 package.yaml과 tasks/CT-*.yaml에 기록한다.
구체적인 실행 순서는 각 execution-guide.md, 에디터 게이트는 editor-guide.yaml을 따른다.

## 전체 순서

| 순서 | WP | 브랜치 | CT | 사람 확인 |
| --- | --- | --- | --- | --- |
| 1 | [WP-P9D4](../changes/WP-P9D4/package.yaml) 항아리 파괴·Loot | feature/destructible-loot | 5 | E01 임시 외형/표현 → E02 기본 공격 → E03 Loot/Pickup → E99 네트워크·Chaos |
| 2 | [WP-D6L2](../changes/WP-D6L2/package.yaml) 분기형 파티 방 전환 | feature/dungeon-layout | 9 | CT-002 순서형 Route 완료 → CT-003 Graph 확장 → E01 3방/분기 → E02 로더 → E03 Exit Marker → E99 3인 분기 전환 |
| 3 | [WP-R8K3](../changes/WP-R8K3/package.yaml) 방 전투 상태·재방문 Spawn | feature/dungeon-room-state | 6 | E01 Spawn Marker → E02 수동 재활성화 → E99 몬스터 Exit 잠금·A→B→A 비재생성 |

WP-P9D4와 WP-D6L2는 서로 독립이다. WP-R8K3은 두 WP의 Merge와 최종 PIE 확인 뒤 최신 master에서 시작한다.
현재 브랜치에는 세 계획을 함께 작성했다. 공통 계획은 별도로 검토·공유한 뒤 구현 브랜치에서 사용하며, 위 브랜치명은 실행 계획이지 생성 완료 기록이 아니다.
동시 작업은 서로 다른 체크아웃과 에디터 인스턴스에서 각 WP 하나만 선택한다. 하나의 작업 디렉터리에서 실행 중 브랜치를 교체하지 않는다.
실제 실행 전 최신 master와 계획 계약을 다시 비교하고 계획 파일 보유 여부를 확인한다.
CT Commit이나 PR 생성만으로 WP를 완료 처리하지 않는다. 실제 PR Merge와 앱의 상태 확인이 필요하다.

## 코드와 에셋의 처리 경계

| 소유 WP | C++ 수정 범위 | 새 테스트 콘텐츠 루트 | 테스트 맵 |
| --- | --- | --- | --- |
| WP-P9D4 | Object/Destructible, Data/Loot, MeleeHitboxComponent, GA_NormalAttack | /Game/LB/Tests/Destructible | Lvl_DestructibleTest |
| WP-D6L2 | Dungeon, Data/Dungeon, LB_GameMode, LB_GameState, LB_PlayerController | /Game/LB/Tests/Dungeon | Lvl_DungeonTransitionTest |
| WP-R8K3 | MonsterBase, DestructibleProp, Dungeon Runtime/Marker, LB_GameMode | /Game/LB/Tests/Dungeon | Lvl_DungeonTransitionTest |

각 CT의 allow가 정확한 파일 권한이며 다른 WP의 코드 경로는 package.yaml의 protected로도 분리한다.
테스트 BP·Data Asset·Geometry Collection·맵 조각·테스트 전용 GameMode는 각 콘텐츠 루트 안에 생성한다.
기존 Lvl_GameplayTest·BP_GameMode·BP_PlayerCharacter·MasterItemTable·BP_DropItem과 Config는 공용 읽기 전용이다.
필요한 테스트 기본값은 WP 전용 BP 파생과 맵 World Settings의 GameMode Override로 지정한다. 기존 BP를 공용 경로에 덮어쓰거나 전역 맵 설정을 바꾸지 않는다.
공용 코드/에셋 수정이 정말 필요해지면 한 WP에 임의 포함하지 않고 별도 선행 변경으로 분리해 공유한다.
공통 색인·대기열·위키는 계획 작성 시 갱신하고 각 구현 브랜치의 반복 수정 대상으로 삼지 않는다. 완료 상태 정리는 담당자가 한 번 통합한다.

## 시작 전 기준 검증

각 브랜치에서 첫 Claude 실행 전에 기준 커밋·일반 LBEditor 빌드 결과·기존 오류/Warning을 기록한다. 이 사전 조건은 사람 실행 절차이며 현재 editor-guide의 CT 이후 게이트가 자동 강제하는 것으로 표시하지 않는다.
항아리 WP는 기존 테스트 맵을 저장하지 않고 Listen Server+Client에서 플레이어 기본 공격이 서버 OnMeleeHit와 기존 피해 경로까지 도달하는지도 확인한다.
실패하면 원인을 기존 코드/에셋 또는 새 변경으로 구분하고, 공용 선행 수정은 별도 범위로 처리한다. 통과하지 않은 기준을 통과로 체크하지 않는다.
각 WP Merge 후 최종 master에서 기존 전투와 각 전용 테스트 맵을 함께 회귀 확인한다.

- Content/Config/Build.cs/uproject 보호 규칙을 유지한다.
- 항아리 C++은 Geometry Collection을 직접 소유하지 않는다. BP 이벤트에서 Chaos 표현을 연결한다.
- 기존 ItemCode·MasterItem·ALB_DropItem을 재사용한다. 새 Pickup/Inventory는 만들지 않는다.
- 던전 방은 서로 붙지 않는 정적 non-World Partition 맵으로 시작하고 두 스트리밍 슬롯 사이를 Fade/서버 Teleport로 전환한다. 게임플레이 Actor 복제와 Spawn은 별도 WP다.
- WP-R8K3은 `RoomId + SpawnId`로 죽은 몬스터와 깨진 Prop만 서버 세션에 기록한다. 생존 몬스터가 있으면 Exit를 잠그고, 재활성화 때 소비 Actor를 Spawn하지 않는다.
- Runtime Monster/Prop과 Prop Loot DropItem은 방 Streaming ULevel을 소유하며 방 활성화 Spawn은 실패 시 전체 롤백한다.
- WP-D6L2의 Graph Cycle·다중 Entry·TargetEntryId를 재사용해 실제 재방문을 검증하되 Graph 생성 규칙은 바꾸지 않는다. 미획득 DropItem은 방 해제 때 사라지고 Prop Loot는 다시 추첨하지 않는다.
- 코드만 빌드된 상태와 에셋 연결 및 PIE 완료를 구분한다. 사용자가 합의한 설계 방향은 Accepted이며 구현·검증 완료를 뜻하지 않는다.

## 기준 문서

- [항아리 설계](../../../LB-wiki/world/destructible-prop.md)
- [던전 방 전환 설계](../../../LB-wiki/world/dungeon-layout.md)
- [던전 방 상태 설계](../../../LB-wiki/world/dungeon-room-state.md)
- 기존 Workflow Manager 작성 기준: LB-wiki의 workflow/workflow-task-authoring.md
