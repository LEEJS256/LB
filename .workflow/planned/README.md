# 근접 몬스터 프로토타입 예정 작업

별도 월드 기능 대기열: [항아리 파괴·Loot 및 정적 던전 조립](world-prototype.md).
아래 내용은 기존 몬스터 계획의 작성 당시 기록이며 새 월드 WP의 실행 순서는 별도 문서를 따른다.

> 상태: 예정 계획. 코드 구현·에셋 연결·빌드·PIE 미실행.

## 보관과 시작

실행 중이거나 예정된 WP는 Workflow Manager 목록에 함께 보이도록 `.workflow/changes/`에 둔다.
이 폴더는 전체 순서와 수동 검증 게이트를 설명하는 사람용 대기열 문서만 보관한다.
현재 앱은 WP 의존성, 에셋 게이트, 전체 WP 순서를 자동 강제하지 않으므로 목록에 보이는 순서와 실행 가능 순서는 같지 않다.
여러 WP를 동시에 표시하되 아래 선행 조건을 확인한 WP 하나만 선택해 실행한다.
코드 계획의 description/goal에는 실행에 필요한 계약만, 사람 절차는 이 문서와 각 가이드에 둔다.

1. 기존 WP-M8H4 PR을 리뷰·Merge한다. 이 문서 작성 시점에는 Merge를 확인하지 않았다.
2. 선행 WP와 아래 에디터 게이트를 완료한 최신 master에서 다음 기능 브랜치를 만든다.
3. Workflow Manager에서 해당 WP만 선택하고 다른 예정 WP는 실행하지 않는다.
4. 최신 코드·심볼·설정과 계획을 대조하고, 해당 브랜치에서 필요한 계획 변경을 Commit한다.
5. CT 실행 → 빌드·Diff·사람 확인 → Commit·Push를 반복한다.
6. 필요한 에셋은 사람이 Editor에서 변경하고 별도로 Git 검토·Commit한다.
7. 최종 코드/에셋 상태로 통합 검증 후 PR → Merge, 그다음 WP로 이동한다.

계획 수립 기준은 2026-10-20의 `feature/melee-monster-facing@3dfa976`이다.
계획만 작성했으며 후속 브랜치와 PR은 생성하지 않았다.
시작할 때마다 기준 차이를 확인한다. 선행 WP의 예정 함수·파일이 없으면 구현부터 시작하지 않는다.

## 전체 순서

| 순서 | WP | 코드 범위 | CT | 중간 검증 / PR 시점 |
| --- | --- | --- | --- | --- |
| 0 | WP-M8H4 (기존) | Hitbox 파생 구조·서버 방향 | 완료 Commit 존재 | 기존 PR 마무리 |
| 1 | [WP-A6R2](../changes/WP-A6R2/package.yaml) | 서버 수동 공격·방향 표현/복제·공용 Team 태그 조회 | 5 | CT-002 후 E01, CT-004 후 E02, CT-005 후 깨끗한 Unity Build / 전체 완료 후 PR 1개 |
| 2 | [WP-S4C8](../changes/WP-S4C8/package.yaml) | StateTree·타깃·추적·공격/Recover·사망 중단 | 7 | CT-001 후 E03, CT-005 후 E04, CT-006 후 E05, CT-007 후 E06 / 전체 완료 후 PR 1개 |

WP 내부 CT 사이에는 Merge를 요구하지 않는다. 작은 변경은 CT별 Commit과 중간 PIE로 확인한다.
WP 사이에서만 앞 WP의 **전체 검증과 PR Merge**를 확인하고 최신 master에서 다음 브랜치를 만든다.
기존 WP-F2V7은 WP-A6R2의 CT-003/004로, WP-T6N3/WP-W9B5/WP-D3P9는 WP-S4C8의 CT-002~007로 통합했다.
중간 에디터 게이트는 유지하며, 통과한 결과를 최종 PR 본문에 함께 기록한다.

## 코드와 에셋의 처리 경계

- `Content/**`, `Config/**`, Build.cs, uproject 보호 규칙은 변경하지 않는다.
- Claude는 C++ Read/Grep/Glob/Edit/Write만 수행한다. Editor 조작이나 에셋 파일 직접 생성은 맡기지 않는다.
- 에셋 연결 전 코드 CT를 Commit해 깨끗한 상태로 만든다.
- 사람이 에셋을 저장하면 기존 Git 도구에서 관련 파일만 검토·Commit한다.
  보호 경로의 에셋 변경을 앱의 코드 CT Commit에 섞지 않는다.
- 에셋 Commit 이후 앱의 통합 검증을 다시 실행하고 PIE 수동 확인을 체크한다.
- Blueprint 노드·Montage·NavMesh 작업을 완료했다고 표시하기 위한 빈 CT Commit은 만들지 않는다.
- 기존 `LB_MeleeMonster.uasset` 경로가 존재한다. 이름만으로 부모·Mesh·데이터 연결을 확정하지 말고
  Editor에서 확인한 뒤 재사용한다. BP_MeleeMonster를 무조건 중복 생성하지 않는다.

## E01 서버 수동 공격·피해

WP-A6R2의 CT-002 Commit 후 수행한다. 통과하면 PR 없이 같은 브랜치에서 CT-003으로 이어간다.

1. 기존 몬스터 BP의 실제 부모를 확인하고 ALB_MeleeMonster 파생으로 사용할 BP 하나를 정한다.
   공통 BP_MonsterBase는 Hitbox 없는 정지 더미로 유지한다.
2. ULB_GA_MonsterMeleeAttack 파생 Ability BP를 만들고 AttackMontage와 DamageEffectClass를 지정한다.
   경로는 에디터에서 실제 GE/몽타주를 확인한다. 현재 피해 GE 파일은 `Content/LB/GAS/GE/GE_NormalAttackDamage.uasset`이다.
3. DA_Monster_Default를 공유하는 더미에 의도치 않게 Ability를 부여하지 않도록
   근접 몬스터용 Data Asset을 별도로 두고 StartupGA·StartupGE를 연결한다.
   Attribute 초기화 GE와 GE_Team_Monster를 유지하고 ATK.Normal 몬스터 공격은 정확히 하나만 부여한다.
4. Mesh Skeleton·AnimBP Slot·Montage 호환성을 확인한다. 공용 MeleeHitWindow Notify State를 사용하고
   SocketName=None, 양수 X 오프셋으로 설정한다.
   원격 몬스터가 화면 밖이어도 서버가 Montage Notify를 평가하도록 Mesh의 애니메이션 Tick 옵션을 확인한다.
5. 서버 전용 테스트 BP 이벤트에서 Player Target으로 RequestMeleeAttack을 한 번 호출한다.
   입력/테스트 이벤트를 제품 전투 API나 자동 AI로 확장하지 않는다.
6. Listen Server + Client 1에서 Monster→Player와 Player→Monster 피해·Health 복제를 확인한다.
   같은 팀·Team 누락·사망 Target에는 피해가 없어야 한다. Team 누락은 Warning을 확인한다.
7. 공격 중 재호출·Montage 누락·중단·Ability 취소를 시험한다.
   Attack 태그와 명중 Delegate가 정리되고 Hitbox가 NoCollision으로 돌아와야 한다.
8. 요청 함수 true를 실제 명중이나 종료 성공으로 기록하지 않는다.

시각적 좌우 방향은 E02에서 검증한다. E01에서도 서버 Hitbox 방향과 피해는 정확해야 한다.
계속 실패하면 CT-003을 시작하지 않고 현재 코드/에셋 연결을 먼저 고친다.

## E02 방향 표현·복제

- WP-A6R2의 CT-003/004는 WP-M8H4의 서버 Hitbox 방향 계약을 복제 원본 + Hitbox 적용값으로 명시적으로 확장한다.
  서로 독립적으로 변경 가능한 두 방향 상태를 만들지 않는다.
- Mesh 기본 회전은 에셋마다 다르므로 우측 기본값과 좌측 180도 차이를 Editor에서 확인한다.
- Target 좌/우/동일 X·미세 X 차이·무효 Target을 테스트하고 서버와 Client의 Mesh·판정이 일치하는지 본다.
- Spawn 직후와 방향 변경 후 늦은 Client 접속에서도 방향이 맞는지 확인한다.
- 복제된 Character 이동의 Mesh 보정이 방향 회전을 되돌리거나 덜컥거리게 하지 않는지 확인한다.
- 플레이어 Facing과 기존 공격은 회귀 검증한다. 추적 중 방향은 이 단계의 범위가 아니다.

## E03 최소 서버 StateTree

WP-S4C8의 CT-001 Commit 후 확인한다. 통과하면 PR 없이 CT-002로 이어간다.

- ALB_MonsterAIController 파생 BP와 StateTreeAIComponentSchema를 사용하는 최소 Tree를 준비한다.
- 몬스터 BP의 AIControllerClass·AutoPossessAI를 연결한다. Character/GAS 코드를 바꾸지 않는다.
- 처음에는 Wait 한 상태로 시작하며 탐색·추적·공격을 동시에 연결하지 않는다.
- 배치된 몬스터와 런타임 Spawn 몬스터에서 준비 완료 후 Tree가 서버에서 한 번만 시작하는지 확인한다.
- Tree 미지정·UnPossess·재Possess·맵 종료에서 충돌·남은 재시도·중복 실행이 없어야 한다.
- 엔진 로컬 UE 5.8 헤더에서 StateTreeAIComponent와 SetStartLogicAutomatically/StartLogic API를 확인했다.
  설치된 다른 엔진 버전에서 실행한다면 시작 전에 계약을 재확인한다.

## E04 탐색·추적·축별 조건

WP-S4C8의 CT-005 Commit 후 확인한다. 통과하면 PR 없이 CT-006으로 이어간다.

1. Tree에 AcquireTarget과 Chase를 추가한다. Controller의 TargetActor를 현재 타깃 기준으로 바인딩한다.
   FindTarget의 과거 Output만 후속 상태에서 계속 읽지 않는다.
2. AcquireTarget에서 FindTarget 성공 시 Chase, 실패 시 짧은 Wait 후 재탐색한다.
3. Chase는 엔진 Move To를 사용한다. 이동 중인 Actor 추적과 실패 시 대기 후 재탐색을 설정한다.
   Move To의 단순 반경 도달만 공격 조건으로 사용하지 않는다.
4. 타깃 상실·사망·Destroy 시 AcquireTarget으로 돌아가는 살아 있는 상위 상태 전이를 둔다.
5. 테스트 레벨 NavMeshBoundsVolume과 생성 상태를 확인한다. 도달 불가능 위치에서는 재시도 폭주가 없어야 한다.
6. CharacterMovement 회전 설정은 BP에서 확인한다. Actor 회전에 의존해 판정 축을 바꾸지 않는다.
7. X/Y/Z 경계 안·경계값·바깥을 각각 확인한다. X가 가까워도 Y/Z가 벗어나면 false여야 한다.
8. 두 플레이어의 거리·사망·퇴장·재진입으로 타깃 선택/상실/재탐색을 확인한다.

현재 문서 설계는 감지 반경/시야/어그로 시스템을 요구하지 않는다. 첫 구현은 살아 있는 플레이어 중 최단 거리다.
공격 범위와 Move To 반경은 Mesh/캡슐·Hitbox에 맞춰 튜닝한다. 본 계획은 수치를 확정하지 않는다.

## E05 공격·회복 반복

WP-S4C8의 CT-006 Commit 후 확인한다. 통과하면 PR 없이 CT-007로 이어간다.

- Attack 진입 시 이동을 중지하고 현재 Target/범위를 다시 확인한다.
- ActivateAbilityAndWait의 거리 파라미터를 E04 Condition과 같은 Tree 설정값으로 바인딩한다.
- 정상 공격 종료만 Recover로 보낸다. 엔진 Wait로 RecoverDuration을 기다린다.
- Recover 후 타깃 유효·범위 안이면 Attack, 범위 밖이면 Chase, 무효면 AcquireTarget이다.
- 활성화 실패·동기 취소·Target 상실·Timeout은 안전하게 정리 후 짧은 대기를 거쳐 재판단한다.
  실패 직후 같은 프레임 무한 Attack 재진입을 만들지 않는다.
- 서로 다른 몬스터 두 마리가 공격할 때 한쪽 종료가 다른 쪽 Task를 완료하지 않아야 한다.
- 공격 시작 직후 취소·즉시 종료·에셋 누락·강제 상태 이탈에서 Delegate/Hitbox가 남지 않아야 한다.
- Task 실행 중만 사용하는 Tick은 종료 시 없어야 하며 Actor Tick을 새로 켜지 않는다.

## E06 사망 우선·최종 완료 기준

WP-S4C8의 CT-007 Commit 후 최종 확인한다. 여기까지 통과한 뒤 AI 전투 PR 하나를 만든다.

- Root에서 Dead 선택/전이를 다른 Alive 상태보다 우선시한다.
  Condition은 ASC State.Death 또는 기존 IsDead를 읽고 사망 bool을 별도 저장하지 않는다.
- Dead는 종료/비전투 상태로 유지하고 탐색·이동·공격으로 다시 돌아가지 않게 한다.
- AcquireTarget, Chase, Attack, Recover 각각에서 몬스터 Health를 서버 피해로 0까지 내려 확인한다.
- 사망 중 Montage·Ability·Hitbox와 이동이 멈추며 반복 사망 알림에도 정리가 중복 실행되지 않아야 한다.
- Target 플레이어 사망/퇴장과 몬스터 자신의 사망을 구분한다. 전자는 재탐색, 후자는 Dead다.
- Listen Server + Client 1에서 이동·좌우 표현·공격 Montage·피해 Health·Death 태그가 맞아야 한다.
- 두 플레이어와 여러 몬스터가 함께 있어도 팀 필터/대상별 1회 명중/구독 정리가 유지돼야 한다.
- 최종 코드/에셋 Commit 기준 일반 빌드와 PIE를 확인하고 PR에 실제 수행 결과를 남긴다.

## 프로토타입 종료와 제외 범위

살아 있는 플레이어 탐색 → NavMesh 추적 → 축별 공격 범위 → 서버 공격 → 종료 대기 → Recover → 재판단,
어느 상태에서든 사망 시 Dead로 중단되면 이번 프로토타입 범위를 마친다.
감지/시야·어그로, 원거리 몬스터, 피격 경직·넉백, 사망 애니메이션·드롭·Destroy·부활,
복잡한 스폰 시스템과 범용 AI 프레임워크는 포함하지 않는다.

## 근거

- LB-wiki의 character/monster-base.md, character/melee-monster.md, ai/melee-monster-state-tree.md.
  현재 설계 상태는 Draft이며 실행 전에 코드·에셋 결과에 맞춰 보완한다.
- [Epic TryActivateAbility](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GameplayAbilities/UAbilitySystemComponent/TryActivateAbility):
  활성화 요청 true가 이후 실행/피해 성공을 보장하지 않으므로 종료 대기를 별도로 둔다.
- [Epic UStateTreeComponent](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GameplayStateTreeModule/UStateTreeComponent):
  자동 시작 제어, Tree 설정과 Logic 시작/종료 계약을 사용한다.
