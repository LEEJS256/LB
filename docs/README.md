# LB 프로젝트 문서

이 디렉터리는 LB의 게임 설계, 공통 기술 정책과 반복 작업 절차를 관리한다. 문서는 게임 영역별로 분류하며, 이 파일의 색인을 통해 현재 기준 문서를 찾는다.

## 조회 시작점

기능별 코드 진입 색인은 별도 [LB-wiki README](../../LB-wiki/README.md)의 `기능별 코드 조회 시작점`을 사용한다.
플레이어·전투·AI·아이템 상세 문서도 위키를 우선 확인한다. 형제 디렉터리 링크는 로컬 체크아웃 기준이다.
항아리·던전 설계 원본도 위키로 통합했다. 아래 World 항목은 원본 안내이며 실행 WP는 LB에서 관리한다. 같은 색인을 양쪽에 복제하지 않는다.
색인의 코드 기준 커밋과 현재 변경 파일을 대조하고 필요한 심볼만 읽는다. `계획` 항목을 구현 파일로 찾지 않는다.

## 문서 구성

- `character`: 플레이어, 몬스터, 이동, 방향과 애니메이션 연동
- `combat`: 기본 공격, 스킬, 피해, 피격과 콤보
- `gas`: ASC 소유권, Attribute, Gameplay Effect와 Gameplay Tag 공통 정책
- `network`: 권한, 복제와 멀티플레이 검증 기준
- `workflow`: 빌드, PIE 테스트와 에셋 연결 같은 반복 작업 절차

아직 문서가 없는 영역의 빈 폴더는 미리 만들지 않는다. 하나의 문서가 여러 영역에 걸치면 문서의 주된 책임을 기준으로 배치하고, 관련 문서에서 링크한다. 같은 내용을 여러 문서에 중복해 기준으로 삼지 않는다.

## 파일명 규칙

- 시스템 문서 파일명은 소문자 kebab-case를 사용한다.
- 예: `character/player-character.md`, `combat/basic-attack.md`
- 공백, 날짜, `final`, `latest`, `v2` 같은 상태·버전 표현은 파일명에 사용하지 않는다.
- 저장소 문서 색인인 `README.md`와 작성 기준인 `_template.md`는 예외다.

## 작성 규칙

시스템 문서는 [`_template.md`](_template.md)를 기준으로 작성하고 다음 다섯 H2 섹션을 유지한다.

1. `Overview`
2. `Key Decisions`
3. `Architecture`
4. `Trade-offs`
5. `Future`

추가 구분이 필요하면 해당 H2 안에 H3를 사용한다. 현재 코드에서 확인한 사실과 아직 구현되지 않은 결정 또는 계획을 구분하고, 문서에는 설계 판단과 그 이유를 남긴다.

`README.md`는 문서 색인, `_template.md`는 작성 기준이므로 위 다섯 섹션 규칙을 적용하지 않는다.

### 문서 상태

각 시스템 문서는 제목 바로 아래에 `> Status: <상태>` 형식으로 현재 단계를 표시한다.

- `Draft`: 작성 또는 논의 중이며 구현 기준으로 확정되지 않음
- `Accepted`: 설계 합의가 끝나 구현 기준으로 사용할 수 있지만 구현 완료를 의미하지 않음
- `Implemented`: 설계가 코드와 에셋에 반영되고 필요한 빌드 및 기본 검증을 완료함
- `Deprecated`: 더 이상 현재 기준이 아니며 신규 작업의 기준으로 사용하지 않음
- `Superseded`: 다른 문서로 대체됨. 본문에 대체 문서 링크를 명시함

상태는 문서의 신뢰 수준과 생명주기를 나타낸다. `Accepted`에서 `Implemented`로 변경할 때는 관련 구현과 검증 결과가 문서 내용과 일치하는지 확인한다.

#### 상태 변경 시점

- `Draft` → `Accepted`: 범위와 제외 범위, 핵심 결정, 책임 분리, 주요 Trade-off가 정리됐고 구현을 막는 미결정 사항이 없을 때 변경한다. 이 시점부터 구현 기준으로 사용할 수 있다.
- `Accepted` 유지: 구현을 시작했거나 일부만 완료한 동안에는 `Accepted`를 유지한다. 구현 진행률은 상태값으로 표현하지 않는다.
- `Accepted` → `Implemented`: 관련 코드와 에셋 반영이 끝나고, 필요한 빌드 및 문서에 적힌 기본 검증을 통과했으며, 문서와 실제 구현이 일치할 때만 변경한다.
- `Accepted` 또는 `Implemented` → `Draft`: 핵심 결정을 다시 논의하거나 실제 구현과 문서가 어긋나 현재 기준으로 신뢰할 수 없을 때 되돌린다.
- 모든 상태 → `Deprecated`: 더 이상 신규 작업의 기준으로 사용하지 않으며 직접적인 대체 문서가 없을 때 변경한다. 중단 이유를 본문에 남긴다.
- 모든 상태 → `Superseded`: 새로운 문서가 현재 기준을 대체할 때 변경하고, 대체 문서 링크를 본문에 남긴다.

오탈자, 표현 개선, 링크 수정처럼 설계 의미를 바꾸지 않는 편집은 상태를 변경하지 않는다. `Implemented` 문서의 핵심 설계를 변경한다면 먼저 `Draft`로 되돌리고 재검토한다.

## 문서 색인

### Character

- [Player Character](character/player-character.md) — `Superseded` — 기존 기록 보존, 현재 기준은 LB-wiki의 Player Character

### World

- [Destructible Prop](world/destructible-prop.md) — `Superseded` — 항아리 단발 파괴, 로컬 Chaos 표현과 ItemCode Loot 드롭 / WP-P9D4
- [Dungeon Layout](world/dungeon-layout.md) — `Superseded` — Data Asset 기반 정적 맵 조각 배치와 각 World의 로드 수명 / WP-D6L2
