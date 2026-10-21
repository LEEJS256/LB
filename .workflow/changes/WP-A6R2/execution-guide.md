# WP-A6R2 수동 전투 실행 지침

사람용 예정 계획이다. Claude context에 넣지 않는다.

## 시작 조건

- WP-M8H4의 PR Merge와 검증을 확인한다. 최신 master에서 `feature/monster-manual-combat`를 만든다.
- Workflow Manager에 함께 표시된 WP 중 WP-A6R2만 선택하고 최신 코드와 계약을 대조한 뒤 필요한 계획 변경을 Commit한다.
- 후속 전체 계획은 `.workflow/planned/README.md`를 따른다. 이 WP 전체에 브랜치 하나·PR 하나를 사용한다.

## CT와 중간 검증

| 순서 | 작업 | 이후 행동 |
| --- | --- | --- |
| CT-001 | ServerOnly 몬스터 공격 Ability | 빌드·검토·Commit |
| CT-002 | Target 기반 수동 공격 요청 | 빌드·검토·Commit 후 E01 수동 공격/피해 확인 |
| CT-003 | 단일 방향 원본과 OnRep | E01 통과 후 실행·빌드·검토·Commit |
| CT-004 | 초기 Mesh 오프셋 기반 좌우 표시 | 빌드·검토·Commit 후 E02 좌우·늦은 접속 확인 |

E01 후에는 PR을 만들거나 master에 Merge하지 않는다. 같은 브랜치에서 CT-003으로 이어간다.
앱은 중간 E 게이트를 자동 강제하지 않으므로 사용자가 통과를 확인하고 다음 CT를 실행한다.
실패는 현재 CT의 수정 요청으로 해결한다. 이미 Commit한 변경의 후속 수정은 같은 브랜치의 추가 수정 Commit으로 남긴다.
CT 번호나 완료 Commit을 다시 만들기 위해 Git 이력을 자동 재작성하지 않는다.

## 에셋·검증·PR

에디터를 닫고 각 코드 CT를 빌드한다. E01/E02 상세 체크리스트는 대기열 README에 있다.
코드 CT를 먼저 Commit한 뒤 에셋을 사람이 Editor에서 연결하고 기존 Git 도구로 별도 검토·Commit한다.
Content 보호 규칙을 열거나 에셋 변경을 앱의 코드 CT Commit에 섞지 않는다.
에셋 변경을 정리한 깨끗한 작업 트리에서 다음 CT를 시작한다.
네 CT와 E01/E02 전체 완료 후 최종 상태로 통합 빌드·PIE → Push → PR을 한 번 생성한다.
PR 제목 예: `feat(monster): 근접 몬스터 수동 전투 구현`.

## 기준과 한계

2026-10-20 LB 3dfa976 기준 예정 계획이다. 구현·빌드·PIE는 아직 수행하지 않았다.
자동 AI와 사망 시 전투 중단은 다음 WP이며, 수동 공격 성공만으로 전체 프로토타입 완료로 기록하지 않는다.
