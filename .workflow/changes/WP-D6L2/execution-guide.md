# WP-D6L2 던전 조각 조립 실행 지침

사람용 문서다. Claude context에 넣지 않는다. 예정 계획이며 코드·에셋·빌드·PIE는 미실행이다.

## 목표와 범위

정적 환경만 담은 non-World Partition 맵 조각 2~3개를 Data Asset의 월드 위치에 조립한다.
동일 맵을 서로 다른 PieceId로 반복 배치한다. 첫 버전의 회전은 0, Scale은 1로 고정한다.
몬스터·항아리·Pickup 같은 게임플레이 Actor, Level Blueprint BeginPlay 스폰은 조각에서 제외한다.
플레이어는 조립 영역 밖의 고정 테스트 플랫폼에서 시작한다. 로드 중 진입 차단 기능은 이번 WP에 없다.

## 설계 근거

- 기준은 2026-10-29 origin/master@2b59a69. 던전 조립 코드는 없다.
- 로컬 UE 5.8 LevelStreamingDynamic.h에서 soft UWorld/FTransform 오버로드와 서버·Client 이름 일치 요구를 확인했다.
- 서버 GameMode가 Layout을 InstanceKey/Pieces로 복사하고 GameState가 확정 목록을 복제한다. 각 World의 비복제 조립기는 Layout을 읽지 않고 목록만 로드한다.
- OnLevelShown은 로컬 가시성 완료다. 서버 Ready를 모든 Client 준비 완료로 취급하지 않는다.
- 설계 원본은 LB-wiki의 world/dungeon-layout.md다. LB docs/world/dungeon-layout.md는 안내 링크다.

## 앱에서 실행하는 순서

| 순서 | CT | 다음 확인 |
| --- | --- | --- |
| 1 | CT-001 Layout/검증 | E01 정적 조각 규격과 DA |
| 2 | CT-002 로컬 조립 수명 | E02 수동 요청·완료/실패·정리 |
| 3 | CT-003 GameState 복제·로컬 실행 | 빌드와 Diff 확인 |
| 4 | CT-004 GameMode 고정 Layout 게시 | E99 동일 배치와 늦은 접속 |

이 WP는 항아리 WP의 코드 API에 의존하지 않지만 현재 계획 보관 브랜치에서 실행하지 않는다.
공통 계획을 포함한 master에서 feature/dungeon-layout을 시작한다. 항아리 WP의 Merge는 선행 조건이 아니며 별도 체크아웃에서 독립적으로 실행할 수 있다.
계획 파일 보유 여부와 기준 차이를 시작 전에 확인하고 던전 WP만 선택한다. 다른 WP 경로는 package.yaml protected와 CT allow로 분리한다.
계획 예약 Commit, CT별 승인 Commit, 사람 에셋 작업, 최종 통합 검증과 PR은 기존 앱 흐름을 따른다.
Codex는 계획을 담당하고 Claude Code가 코드를 구현한다. 자동 Commit/Push/PR은 하지 않는다.

## 이번 WP 확인 항목

첫 Claude 실행 전 기준 커밋의 일반 LBEditor 빌드와 기존 오류/Warning을 기록한다. 현재 editor-guide의 CT 이후 게이트가 이 사전 조건을 자동 강제하는 것은 아니다.
새 맵 /Game/LB/Tests/Dungeon/Lvl_DungeonAssemblyTest와 조각·Layout DA·테스트 BP는 모두 /Game/LB/Tests/Dungeon에 둔다.
공용 BP_GameMode·Lvl_GameplayTest·플레이어 BP·Config는 수정하지 않는다. 별도 BP_DungeonTestGameMode에서 설정을 지정하고 전용 맵의 World Settings GameMode Override만 변경한다.

E01/E02/E99의 정확한 절차는 editor-guide.yaml을 따른다.
원점 기준 바닥 높이와 X 진행축을 통일하고 문/벽 겹침은 제작 단계에서 확인한다.
CT-002 테스트 때만 조립기를 수동 배치한다. 최종 단계에서는 GameState가 Persistent Level에 로컬 조립기를 생성하므로 수동 배치 조립기를 제거한다.
BP_DungeonTestGameMode의 GameStateClass가 ALB_GameState인지 확인하고 StartupDungeonLayout을 연결한다. 조립기는 월드 Transform을 그대로 사용한다.
고정 Layout→확정 배치 목록의 변환만 현재 구현한다. 향후 서버 랜덤 생성은 같은 목록을 만들고 GameState/조립기는 재사용한다.
프로토타입은 조각 1..32개와 World당 게시 1회다. 대규모 목록 분할 전송·런타임 교체·복잡한 생성기는 만들지 않는다.
LoadLevelInstance 반환 성공과 실제 표시 완료를 별도로 확인한다.
테스트용 짧은 Timeout으로 실패 정리 후 남은 Streaming 객체가 제거되는지 확인한다.
Cook/패키지의 Soft 맵 포함 여부는 별도 확인 대상이다. PIE 통과를 패키지 검증으로 기록하지 않는다.

## 후속 PIE와 제외 범위

런타임 재조립, 랜덤 Seed/가중치, 연결 포트/겹침 탐지, NavMesh/전투 Spawn,
복제 Actor가 포함된 조각, 전원 준비 배리어/입력 잠금, World Partition 통합은 후속이다.
게임플레이 Actor를 넣기 전에 별도 작은 WP에서 동적 레벨의 Actor 복제/늦은 접속/해제 수명을 검증한다.
