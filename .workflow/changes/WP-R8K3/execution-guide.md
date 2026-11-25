# WP-R8K3 던전 방 상태 실행 지침

사람용 문서다. Claude context에 넣지 않는다.

## 목표와 시작 조건

방에 직접 몬스터와 항아리 Actor를 놓는 대신 안정적인 `SpawnId` Marker를 둔다. 서버가 현재 방 활성화 때 Actor를 생성하고, 몬스터 사망과 Prop 파괴 결과를 `RoomId`별 세션 상태에 기록한다. 같은 `RoomId`가 다시 활성화되면 기록된 Actor는 다시 생성하지 않는다.

WP-P9D4 항아리/Loot와 WP-D6L2 분기·순환 방 전환이 master에 Merge되고 각각의 최종 PIE 결과가 확인된 뒤 최신 master에서 `feature/dungeon-room-state`를 시작한다. WP-D6L2 실행 중인 `feature/dungeon-layout`에서 이 WP를 함께 실행하지 않는다.

## 결정된 경계

- 상태 키는 현재 Dungeon InstanceKey 세션 안의 `RoomId + SpawnId`다.
- `RoomId`는 방 Template 이름이 아니라 이번 Route에 생성된 방 노드의 고유 ID다. 같은 Level을 여러 노드가 사용해도 상태를 공유하지 않는다.
- 같은 방의 Monster/Prop Marker는 종류와 무관하게 `SpawnId`가 고유해야 한다.
- 죽은 몬스터와 깨진 Prop만 소비 상태로 저장한다. 살아 있는 몬스터의 체력·AI 위치와 온전한 Prop 상태는 저장하지 않는다.
- 마지막 몬스터가 죽기 전에는 모든 Exit를 잠근다. Exit Actor가 몬스터를 직접 검색하지 않는다.
- 깨진 Prop의 Loot는 다시 추첨하지 않는다. 방을 떠날 때 줍지 않은 DropItem은 저장하지 않고 방과 함께 사라진다.
- Monster/Prop은 Marker Transform에 AlwaysSpawn하며 모두 해당 Streaming ULevel을 소유한다. Prop이 만든 DropItem도 Prop과 같은 ULevel에 생성해 방 해제 수명을 따른다.
- 방 활성화는 전검증 뒤 원자적으로 수행한다. 실제 Spawn 또는 delegate 연결 하나라도 실패하면 이번 시도의 Actor·구독을 전부 롤백하고 누적 소비 상태는 변경하지 않는다.
- 상태는 서버 메모리에만 유지한다. SaveGame·서버 재시작 복구는 후속이다.
- WP-D6L2의 방향 Cycle·다중 Entry·`TargetEntryId`를 그대로 사용한다. 이 WP는 Graph를 다시 설계하지 않고 실제 순환 링크의 같은 `RoomId` 재방문에 상태 복구를 연결한다.
- 상태는 서버 메모리 전용이므로 Listen Server 호스트 또는 서버 프로세스가 종료되면 현재 던전 상태도 사라진다. Host Migration·Backend 체크포인트는 후속이다.

## 실행 순서

| 순서 | CT | 다음 확인 |
| --- | --- | --- |
| 1 | CT-001 몬스터 사망 완료 신호 | 서버 1회 신호와 기존 사망 회귀 |
| 2 | CT-002 Prop 파괴 완료 신호 | Loot 전 기록 순서와 기존 항아리 회귀 |
| 3 | CT-003 Spawn Marker | E01 안정 ID와 방 배치 |
| 4 | CT-004 Runtime Manager | 일반 빌드와 Diff 확인 |
| 5 | CT-005 Exit 활성 상태 | 빌드와 Diff 확인 |
| 6 | CT-006 GameMode 통합 | PR 준비 단계에서 E02 독립 검증과 E99 네트워크 통합 검증 |

Codex는 계획과 정합성 확인을 담당하고 Claude Code가 C++을 구현한다. 자동 Commit·Push·PR은 하지 않는다.

## 에셋 구성

`/Game/LB/Tests/Dungeon`의 기존 테스트 맵과 BP를 재사용한다. 공용 `BP_GameMode`, `Lvl_GameplayTest`, 플레이어 BP와 Config는 수정하지 않는다.

Room A에는 서로 다른 `SpawnId`의 Monster Marker와 Prop Marker를 배치한다. Marker의 Class에는 테스트용 몬스터 BP와 `/Game/LB/Tests/Destructible/BP_DestructiblePot`을 연결한다. 실제 Actor를 방 맵에 직접 배치하거나 Level Blueprint에서 Spawn하지 않는다.
Marker는 서로·벽·바닥·Entry·Exit Trigger와 겹치지 않게 배치한다. Monster BP는 `AIControllerClass`, `Auto Possess AI = Placed in World or Spawned`, Monster Data와 StateTree 연결을 기존 몬스터 테스트 기준대로 유지한다.
Room B/C처럼 게임플레이 Spawn Marker가 없는 방도 유효하다. 빈 방은 생존 몬스터가 없는 상태로 활성화되고 기존 Exit 전환을 그대로 허용한다.

## Runtime Manager 독립 검증 (E02)

E02는 CT-006 커밋 이후 PR 준비 단계에서 수행한다. Runtime Manager의 활성화·해제는 일반 C++ 호출이므로, 별도 서버 측 C++ 테스트 경로가 필요하다. 테스트 경로를 제품 API로 공개하지 않는다.

먼저 중복 SpawnId, None ID, 무효 Class가 Spawn 전에 거부되는지 확인한다. 유효한 구성에서는 일부 Actor가 생성된 뒤 Spawn 또는 delegate 연결 실패를 별도로 유도해, 이번 시도의 Actor·구독만 정리되고 누적 소비 상태가 유지되는지 확인한다.

재활성화는 다음 순서로 확인한다.

1. 몬스터 2개 중 1개만 죽이고 Prop 2개 중 1개만 파괴한다.
2. 현재 방 Runtime을 비활성화한 뒤 같은 `RoomId`와 표시 ULevel로 다시 활성화한다.
3. 죽은 몬스터와 깨진 Prop은 생성되지 않고, 생존했던 몬스터와 온전했던 Prop만 초기 상태로 생성되는지 확인한다.
4. 깨진 Prop의 Loot가 다시 추첨되지 않는지 확인한다. 이전 방의 미획득 DropItem이 사라지는 것은 이번 정책상 정상이다.

## 실제 재방문 확인

E99에서는 Room A의 몬스터를 모두 처치해 Exit를 연 뒤 Prop 하나만 파괴하고 `Room_A → Room_B → Room_A`로 이동한다. 돌아온 Room A에서는 죽은 몬스터와 깨진 Prop이 생성되지 않고 온전한 Prop만 초기 상태로 생성돼야 한다. 목적지 `TargetEntryId` 도착과 기존 도착 Exit 억제도 유지돼 즉시 Room B로 되튕기지 않아야 한다.

Exit 잠금 때문에 생존 몬스터를 남긴 방의 재활성화는 PR 준비 단계의 E02에서 별도로 확인한다.

## 제외 범위와 후속

Graph 생성 알고리즘 변경, 몬스터 중간 체력 저장, DropItem 보존, 보스/웨이브 Spawn, SaveGame, Host Migration과 Backend 복구는 후속 WP로 분리한다. 향후 랜덤 생성기도 각 생성 방 노드에 고유 `RoomId`를 부여하고 이 WP의 `RoomId + SpawnId` 상태를 재사용한다.
