# WP-B7C1 실행 지침

사람용 예정 계획이다. CT의 `context`에는 넣지 않는다.

## 시작 조건

- 현재 `.workflow/changes/WP-M8H4` 작업과 관련 PR을 먼저 마무리한다.
- 최신 `master`에서 `refactor/battle-component` 브랜치를 만든다.
- Workflow Manager에 함께 표시된 WP 중 WP-B7C1만 선택하고 최신 코드와 계획을 대조한 뒤 필요한 계획 변경을 Commit한다.
- Unreal Editor를 닫고 헤더·기본 서브오브젝트 변경을 일반 빌드로 검증한다.

## 목표와 범위

현재 Normal/Strong 공격의 요청 경로만 `ULB_BattleComponent`로 옮긴다.
PlayerController는 입력 전달만 담당하고 Component가 기존 PlayerState ASC를 통해 Gameplay Tag 기반 Ability 활성화를 요청한다.
기존 공격 Ability, Montage, Hitbox, 피해와 에셋 연결은 변경하지 않는다.

설계 기준은 2026-10-21의 LB-wiki `combat/battle-component.md` Draft와 현재 코드다.
실행 전 최신 `master`에서 `ALB_PlayerController`, `ALB_PlayerCharacter`와 공격 태그 계약을 다시 확인한다.

## CT 실행 순서

| 순서 | 작업 | 확인 후 행동 |
| --- | --- | --- |
| CT-001 | BattleComponent와 공격 요청 API | 빌드·Diff 검토·Commit |
| CT-002 | PlayerCharacter에 Component 장착 | 빌드·Diff 검토·Commit |
| CT-003 | Normal 공격 요청 경로 이전 | 빌드·Diff 검토·Commit |
| CT-004 | Strong 공격 요청 경로 이전 | 빌드·Diff 검토·Commit 후 통합 PIE |

각 CT는 독립 빌드가 가능한 상태로 마친다. 앞 CT가 실패하면 다음 CT로 넘어가지 않는다.
기존 Content·Config·Build.cs·uproject는 수정하지 않는다.

## 구현 확인

- `ULB_BattleComponent`는 새 ASC를 생성·소유하거나 Tick·복제를 켜지 않는다.
- PlayerState 도착 전 ASC가 없을 수 있으므로 BeginPlay 한 번만으로 유효하지 않은 포인터를 영구 캐시하지 않는다.
- Normal은 `TAG_ATK_Normal`, Strong은 `TAG_ATK_Strong` 네이티브 태그를 사용한다.
- Strong 경로의 `TAG_Input_RightClick` Gameplay Event는 실제 에셋 의존을 확인하기 전까지 보존한다.
- `RequestAttackByTag()` 반환값은 활성화 요청 결과이며 명중·피해·Ability 완료 성공으로 해석하지 않는다.
- Character와 Controller에서 콤보 상태, 입력 버퍼, Combo Data와 Montage Section 정책을 추가하지 않는다.

## 최종 검증

1. 에디터 종료 상태에서 LBEditor 통합 빌드를 실행한다.
2. BP_PlayerCharacter를 열어 BattleComponent 기본 서브오브젝트가 보이고 기존 컴포넌트·에셋 연결이 유지되는지 확인한다.
3. Listen Server + Client 1에서 각 소유 플레이어의 X 입력이 `ATK.Normal`을 한 번 요청하는지 확인한다.
4. 양쪽에서 기존 공격 Montage, 이동/방향 잠금, Hit Window와 서버 피해 복제가 유지되는지 확인한다.
5. Z 입력의 기존 `Completed` 시점에 Strong 요청과 `Input.RightClick` 이벤트가 한 번 전달되는지 확인한다.
6. ASC 미준비, Avatar 불일치와 해당 Ability 미부여 상태에서 충돌하거나 로컬 결과를 확정하지 않는지 확인한다.
7. 최종 Diff에 허용된 C++ 파일과 이 WP 계획 외 에셋·빌드 설정 변경이 없는지 확인한다.

Z용 Strong Ability나 에셋이 아직 연결되지 않았다면 활성화 성공이나 공격 연출을 이번 WP 완료 조건으로 기록하지 않는다.
대신 기존 이벤트·태그 요청 경로가 Component로 동일하게 이전됐는지 로그 또는 디버거로 확인한다.

## 제외 범위

콤보 인덱스, 입력 버퍼, Combo Window, Combo Data Asset/Data Table, Montage Section 구성·전환,
Normal→Strong 파생 규칙, 공격 GA 상속 리팩터링, 신규 Ability·Gameplay Effect·Input Action·에셋 생성,
AI 공격 공통화와 기존 공격 피해 공식 변경은 포함하지 않는다.
