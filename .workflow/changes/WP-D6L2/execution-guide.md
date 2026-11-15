# WP-D6L2 파티 방 전환 실행 지침

사람용 문서다. Claude context에 넣지 않는다. 2026-11-15 합의로 기존의 연속 정적 조각 조립 계획을 방 단위 Fade 전환으로 교체했다.

## 목표와 범위

별도 non-World Partition `.umap`인 Room A/B/C를 Layout Data Asset의 Exit 연결 Graph로 로드한다.
Room A의 Exit_Left는 Room B, Exit_Right는 Room C로 연결한다. 한 Exit에 파티 과반수가 들어오면 카운트다운하고, 전원이 들어오면 대기 또는 카운트다운 중에도 해당 목적지 전환을 잠근다.
선택 목적지가 서버와 참가 Client에 준비된 뒤 Fade Out, 서버 파티 전체 Teleport, 이전 방 해제, Fade In을 수행한다.

몬스터·항아리·Pickup·방 클리어·랜덤 방 선택·사망 참가 규칙·실패 재시도 UX는 포함하지 않는다.

## 현재 기준과 재시작 주의

- 브랜치는 `feature/dungeon-layout`이다.
- 2026-11-15 현재 기존 연속 조립 계약의 CT-001이 `feature/dungeon-layout@a3902b4`에 Commit됐다.
- 기존 CT-001은 완료 기록으로 유지한다. CT-002가 Piece/Transform을 순서형 Room Route로 교체했고 Workflow Manager 검증까지 통과했다. 새 CT-003이 이 Route를 Room/Exit Graph로 확장하며 기존 Commit을 임의로 되돌리지 않는다.
- `ALB_GameMode`와 `ALB_GameState`는 빈 골격이며 `ALB_PlayerController`에 던전 Fade·준비 보고 경로는 없다.
- 첫 재실행 전에 일반 LBEditor 기준 빌드와 기존 오류/Warning을 기록한다.

## 설계 근거

- 방은 서로 지형으로 접합하지 않고 두 런타임 슬롯을 번갈아 사용한다.
- Layout은 StartRoomId와 Room별 soft UWorld·ExitId→TargetRoomId Graph를 작성하고 슬롯 Transform과 진행 상태는 런타임 책임이다.
- 전환 권한은 서버 GameMode, 공유 상태와 로컬 로더 수명은 GameState, 준비 보고와 Fade 표현은 PlayerController가 맡는다.
- 엄밀한 과반수는 `Total / 2 + 1`이다. 2인 파티에서는 전원 조건과 같아 카운트다운이 발생하지 않는다.
- 전원 진입은 카운트다운보다 우선하며 별도 안내 문구 없이 전환 준비로 넘어간다.
- 설계 원본은 LB-wiki의 `world/dungeon-layout.md`다.

## 앱에서 실행하는 순서

| 순서 | CT | 다음 확인 |
| --- | --- | --- |
| 1 | CT-001 기존 연속 조립 데이터 | 완료 기록 유지, 재실행하지 않음 |
| 2 | CT-002 순서형 Room Route | 완료/검증 기록 유지 |
| 3 | CT-003 Room/Exit Graph 확장 | E01 방 A/B/C와 분기 Layout DA |
| 4 | CT-004 단일 방 로더 | E02 수동 로드·완료/실패·해제 |
| 5 | CT-005 Entry/ExitId Marker | E03 방 Marker·Overlap |
| 6 | CT-006 GameState Graph·두 슬롯 | 빌드와 Diff 확인 |
| 7 | CT-007 PlayerController 준비/Fade | E04 로컬 Fade·카운트다운 표현 |
| 8 | CT-008 GameMode Graph·Barrier | E05 첫 방 준비 Barrier |
| 9 | CT-009 Exit별 과반수·분기 전환 | E99 3인 최종 네트워크 검증 |

항아리 WP Merge는 선행 조건이 아니다. 공용 `BP_GameMode`, `Lvl_GameplayTest`, 플레이어 BP와 Config는 수정하지 않는다.
Codex는 계획과 정합성 확인을 담당하고 Claude Code가 C++을 구현한다. 자동 Commit·Push·PR은 하지 않는다.

## 에셋 구성

모든 테스트 에셋은 `/Game/LB/Tests/Dungeon` 아래에 둔다.

```text
Maps/Lvl_DungeonTransitionTest
Maps/Lvl_DungeonRoom_A
Maps/Lvl_DungeonRoom_B
Maps/Lvl_DungeonRoom_C
Data/DA_DungeonRouteTest
Blueprint/BP_DungeonTestGameState
Blueprint/BP_DungeonTestGameMode
Blueprint/BP_DungeonRoomLoaderTest  # CT-004 수동 확인 때만 필요
UI/WBP_DungeonCountdownTest
```

방 A/B/C는 서로 붙이지 않는다. 각 방의 로컬 원점과 바닥 기준을 통일한다. Room A에는 `ALB_DungeonRoomEntry` 하나와 Exit_Left/Exit_Right 두 개를 두고 Room B/C에는 Entry만 둔다.
Persistent 테스트 맵에는 조립된 방을 미리 배치하지 않고 안전 플랫폼·PlayerStart·전역 조명만 둔다.
`BP_DungeonTestGameState`의 Slot A/B Transform은 방 크기보다 충분히 떨어뜨리고, 동일 값이 서버와 Client에서 사용되게 한다.

## 카운트다운과 UI 확인

카운트다운은 3인 테스트가 필요하다. Listen Server 1명과 Client 2명을 사용한다.

- 한 Exit 1/3명: 대기, 카운트다운 없음
- 한 Exit 2/3명: 해당 Exit 카운트다운 시작
- 2명 중 1명 이탈: 즉시 취소와 UI 숨김
- 다시 2/3명: 전체 시간으로 재시작
- Exit_Left 2/3명 Timer 완료: Exit 밖 1명을 포함한 전원이 Room B로 이동
- Exit_Right 2/3명 Timer 완료: Exit 밖 1명을 포함한 전원이 Room C로 이동
- Left/Right로 분산돼 어느 쪽도 과반수가 아니면 대기하고 서로 다른 Exit 인원을 합산하지 않음
- 카운트다운 중 3/3명: Timer 취소 후 즉시 전환 잠금
- 대기 중 3/3명: 카운트다운 없이 즉시 전환 잠금

UI는 CountingDown일 때만 현재/전체 인원과 남은 시간을 보여 준다. "모든 파티원이 모였습니다"와 `0초` 문구는 만들지 않는다.

## 로드와 이동 확인

RoomLoader 반환 성공과 실제 `OnLevelShown` 완료를 구분한다.
선택된 목적지 방만 현재 방과 다른 슬롯에 준비한 뒤 Fade·서버 Teleport·이전 방 해제 순서로 처리한다.
서버와 모든 참가 Client의 준비 보고가 모이기 전에 Pawn을 이동하거나 Fade In하지 않는다.
로드 실패·Timeout은 Failed 상태와 로그로 드러나야 하며 파티를 빈 공간으로 이동시키지 않는다.
첫 방과 이후 방 모두 Entry가 도착 기준이며 여러 Pawn은 설정된 Y 간격으로 배치한다.

## 제외 범위와 후속

전환 중 새 접속자는 현재 Barrier에 추가하지 않는다. 안정된 현재 방을 복구하는 늦은 접속만 확인한다.
사망·관전자·부활 규칙, 방 클리어 잠금, 실패 재시도, 강제 퇴장, Cycle/되돌아가기, 랜덤 Graph, 게임플레이 Actor 수명과 방별 NavMesh는 후속이다.
Cook/패키지의 Soft 맵 포함 여부도 별도 확인 대상이며 PIE 통과를 패키지 검증으로 기록하지 않는다.
