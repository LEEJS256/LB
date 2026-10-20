# WP-M8H4 실행 지침

사람용 문서다. CT의 `context`에는 넣지 않는다.

## 목표와 범위

근접 몬스터의 Hitbox 소유 구조와 서버 X 공격 방향 API를 두 CT로 구현한다.
이번 WP의 완료는 C++ 기반 추가까지이며 실제 근접 공격이나 AI 동작 완료를 의미하지 않는다.
기존 몬스터 GAS와 플레이어 공격은 유지한다.

## 설계 근거

- 코드 기준: `origin/master@dd22f96b99cf9306fcfcbeef38c58bfbe6dc111c`.
  이전 캐릭터 정리 브랜치의 미병합 변경에는 의존하지 않는다.
- 2026-10-20에 LB-wiki의 Monster Base, Melee Monster, Melee Monster StateTree,
  Basic Attack Damage, Melee Hit Window 문서를 확인했다. 몬스터 설계는 아직 Draft이며
  이번에는 Hitbox와 방향 계약만 실행 범위로 정했다.
- 공통 몬스터는 자체 ASC를 유지하고 근접 파생 클래스만 Hitbox를 가진다.
  공용 Hitbox와 Notify를 재사용하며 별도 Target 저장이나 방향 복제는 만들지 않는다.
- 기존 기본 공격 Ability는 플레이어 방향만 연결한다. 이번 API 추가와 실제 몬스터 공격 연결은 별도다.
- 공유 작성 기준은 LB-wiki의 `workflow/workflow-task-authoring.md`에 둔다.

## 앱에서 실행하는 순서

1. LB 저장소를 열고 `feature/melee-monster-facing` 브랜치와 WP-M8H4를 확인한다.
2. **계획 변경 Commit**으로 이 WP 계획과 프로젝트 설정을 기록한다.
   브랜치는 이미 만들어져 있으며 **작업 시작**만 눌러서는 미커밋 계획이 기록되지 않는다.
3. Unreal Editor를 닫고 CT-001 **Claude 실행** → 자동 빌드 → Diff 확인 → **Commit 승인**.
4. CT-002도 반복한다. 실패하면 로그 첨부 수정 요청으로 같은 세션에서 이어간다.
5. **PR 준비**에서 통합 빌드와 아래 확인을 마친 뒤 Push → Draft PR 생성.
   현재 solo 모드이므로 중간 Push는 선택이다. 한 WP는 한 명이 담당한다.

## 이번 WP 확인 항목

- 두 CT의 실제 빌드 결과와 allow 두 파일만 변경된 Diff를 확인한다.
- 최종 빌드 후 에디터에서 LB_MeleeMonster 클래스를 확인한다.
- 기존 BP_MonsterBase가 정상 로드되고 Hitbox 없는 구조를 유지하는지 확인한다.
- 앱의 검증 결과와 사람 확인을 근거로 완료 처리한다. 빌드·PIE 미실행은 통과가 아니다.

## 후속 PIE 체크리스트

아래 항목은 후속 에셋·Ability 연결 작업에서 검증하며 이번 WP에서 통과로 기록하지 않는다.

- Unreal Editor에서 BP_MeleeMonster를 만들고 Mesh·AnimBP·MonsterData를 연결한다.
- 서버에서 Target 오른쪽/왼쪽은 +1/-1, X 일치·무효·자기 자신·사망은 방향 유지인지 확인한다.
- 클라이언트의 설정 호출이 서버 방향을 변경하지 않는지 확인한다.
- SocketName=None인 Hitbox를 서버에서 잠깐 열어 좌우 배치와 ForceEndHitWindow 정리를 확인한다.
- 공격 Ability·Montage를 연결한 뒤 Listen Server + Client 1에서 피해·팀·사망 필터를 확인한다.
- 수동 공격 검증 후 StateTree 탐색·추적·공격 결정을 연결한다.

## 제외 범위

공격 Ability 구현, Blueprint·Montage·Data Asset·맵 저장, 실제 피해 및 방향 표현 복제,
AI·NavMesh·StateTree, 사망 연출, 기존 코드 리팩터링, 빌드·플러그인 설정 변경은 제외한다.
현재 Content 보호 규칙을 우회하지 않는다. 에셋 저장은 후속 작업으로 분리한다.
