# WP-S4C8 AI 전투 실행 지침

사람용 예정 계획이다. Claude context에 넣지 않는다.

## 시작 조건

- WP-A6R2 전체와 E01/E02 검증, PR Merge를 확인한다.
- 최신 master에서 `feature/monster-ai-combat`를 만들고 Workflow Manager에 함께 표시된 WP 중 WP-S4C8만 선택해 필요한 계획 변경을 Commit한다.
- 앞 WP의 Ability·수동 공격·방향 복제 계약이 있는지 확인한다.
- 이 WP 전체에 브랜치 하나·PR 하나를 사용한다. CT 사이에 Merge를 요구하지 않는다.

## CT와 중간 검증

| 순서 | 작업 | 이후 행동 |
| --- | --- | --- |
| CT-001 | 서버 AIController·Tree 수명 | 빌드·검토·Commit 후 E03 최소 Tree 시작 확인 |
| CT-002 | Controller Target 소유/조회 | 빌드·검토·Commit |
| CT-003 | 살아 있는 플레이어 탐색 Task | 빌드·검토·Commit |
| CT-004 | 현재 Target 유효성 Condition | 빌드·검토·Commit |
| CT-005 | X/Y/Z 공격 범위 Condition | 빌드·검토·Commit 후 E04 탐색·추적 확인 |
| CT-006 | 공격 실행별 종료 대기 Task | 빌드·검토·Commit 후 E05 Attack/Recover 확인 |
| CT-007 | 사망 태그 구독·전투 정리 | 빌드·검토·Commit 후 E06 최종 검증 |

E03/E04/E05 사이에는 PR을 만들지 않는다. 같은 브랜치에서 다음 CT를 진행한다.
`editor-guide.yaml`의 E03~E06은 지정 CT 이후 또는 PR 전에 앱이 표시하며, 사람이 항목을 확인하기 전까지 뒤 CT 또는 PR 생성을 차단한다.
체크는 확인 기록일 뿐 빌드·PIE·Commit을 대신하지 않는다.
실패는 수정 요청 또는 같은 브랜치의 추가 수정 Commit으로 처리하고 자동 이력 재작성은 하지 않는다.

## 에셋·검증·PR

E03~E06의 실행 항목은 `editor-guide.yaml`, 상세 설계 근거는 `.workflow/planned/README.md`에 있다.
코드 CT를 먼저 Commit한 뒤 StateTree·BP·NavMesh는 사람이 Editor에서 연결해 별도 검토·Commit한다.
Content 보호는 유지한다. 에셋 변경을 정리한 깨끗한 작업 트리에서 다음 CT를 시작한다.
공격 Task에 쓰는 범위 값은 Condition과 같은 Tree 설정에서 바인딩한다.
실패/Target 상실/사망 전이와 재시도 대기를 빠뜨리지 않는다.
일곱 CT와 E03~E06이 모두 끝나면 최종 상태로 통합 빌드·네트워크 PIE → Push → PR을 한 번 생성한다.
PR 제목 예: `feat(monster): StateTree 기반 자동 전투 구현`.

## 기준과 한계

2026-10-20 LB 3dfa976 기준 예정 계획이다. 미래 심볼은 앞 CT 또는 WP의 산출물이다.
실제 에셋·빌드·PIE 미검증 상태이며, 최종 프로토타입 완료는 E06 통과로 판단한다.
사망 애니메이션·Collision·Destroy·드롭·부활과 감지/어그로는 포함하지 않는다.
