# Player Character

> Status: Accepted

## Overview

LB의 플레이어 캐릭터를 멀티플레이 기반 2.5D 벨트스크롤 액션 RPG에 맞게 구현하기 위한 1차 기준을 정의한다. 참고 방향은 던전앤파이터, 던전앤드래곤, 드래곤즈 크라운이다.

### 범위

- 방향키 기반의 직접 이동
- X/Y 평면의 벨트스크롤 이동과 대각선 속도 정규화
- +X/-X 두 방향의 캐릭터 바라보기
- 방향 잠금과 이동 잠금의 분리
- GAS 기반 행동 상태 관리
- 서버 권한형 멀티플레이 구조와 최소 네트워크 검증
- 첫 기본 공격 Ability로 이어질 입력·상태 확장 지점

### 제외 범위

- 클릭 이동과 NavMesh 기반 이동
- 세션, 로비, 매치메이킹
- 전용 서버 배포
- 스킬 로드아웃과 런타임 스킬 교체 시스템
- Mover, Motion Matching, Iris 도입
- 전체 전투 시스템이나 전체 캐릭터 계층의 선행 구현

기존 `ALBCharacter`, `ALBPlayerController`의 클릭 이동 및 NavMesh 코드는 Epic 템플릿 참고용으로만 유지한다. LB 신규 플레이어 구조는 해당 코드에 의존하지 않는다.

## Key Decisions

### 좌표계와 이동

- X축은 화면 좌우이자 스테이지의 주 진행 방향이다.
- Y축은 화면 안쪽/바깥쪽으로 이동하는 깊이 방향이다.
- Z축은 점프, 낙하, 띄우기 등 높이 표현에 사용한다.
- 일반 이동은 `ACharacter`와 `CharacterMovementComponent`를 사용한다.
- 입력 벡터는 X/Y 평면에서 정규화해 대각선 이동이 축 단독 이동보다 빨라지지 않게 한다.
- 캐릭터는 +X 또는 -X 방향만 바라본다.
- 마지막 유효 수평 입력의 X 부호를 기준으로 좌우 방향을 유지한다.
- Y축 입력만 있는 동안에는 기존 좌우 방향을 유지한다.
- 월드 X/Y 이동축과 고정 카메라 기준을 유지하기 위해 Actor와 Capsule은 회전시키지 않고 Skeletal Mesh만 좌우로 회전한다.
- Actor의 Forward는 항상 실제 바라보는 방향을 나타내지 않으므로 공격, 투사체와 방향성 판정은 `GetActorForwardVector()`가 아니라 `CurrentFacing`의 +X/-X 방향을 기준으로 계산한다.
- `State.Movement.FacingLocked`가 활성화된 동안에는 이동 입력과 무관하게 좌우 방향을 변경하지 않는다.
- 좌우 방향은 서버 권한을 기준으로 다른 클라이언트에도 일관되게 보여야 한다.

### 입력

- LB 전용 `IA_Move`와 `IMC_Player`를 사용한다.
- `IA_Move` 값 형식은 2D Axis다.
- 현재 프로토타입은 방향키만 X/Y 입력으로 매핑한다.
- WASD는 방향키와 동시에 활성화하지 않고, 향후 입력 선택 옵션이 필요할 때 별도 매핑으로 제공한다.
- 입력 수집은 로컬 `ALB_PlayerController`가 담당한다.
- 캐릭터는 전달받은 입력을 X/Y 월드 이동으로 해석한다.
- 클릭 이동용 Input Action과 Input Mapping Context에는 의존하지 않는다.
- 이후 별도 작업에서 `IA_Attack`을 추가하고 기본 공격 Ability 1종의 활성화 입력으로 연결한다.

### 카메라

- `BP_PlayerCharacter`는 벨트스크롤 플레이를 위한 고정 원근 카메라를 사용한다.
- 카메라는 캐릭터의 좌우 방향 전환과 함께 회전하지 않는다.
- 화면 범위 조정은 카메라 붐의 `타깃 암 길이`를 우선 사용하며, 구체적인 값은 Blueprint에서 튜닝한다.

### 방향 잠금과 이동 잠금

- 방향 잠금은 `State.Movement.FacingLocked`로 표현한다.
- 일반 이동 차단은 `State.Movement.Blocked`로 표현한다.
- 첫 기본 공격은 두 태그를 모두 사용해 방향 전환과 일반 이동을 차단한다.
- 공격 중에는 바라보는 방향을 바꾸지 않는다.
- 향후 공격이나 스킬은 필요에 따라 방향만 잠그거나, 이동만 막거나, 둘 다 허용할 수 있어야 한다.
- 입력 자체를 전역적으로 제거하기보다 Ability의 활성 상태와 Gameplay Tag를 기준으로 캐릭터가 각 규칙을 적용한다.

### 상태 분류

이동에서 자연스럽게 파생되는 상태와 권한 있는 게임플레이 상태를 구분한다.

| 분류 | 상태 | 관리 방식 |
| --- | --- | --- |
| 애니메이션 파생 | Idle, Run, Airborne | AnimBP가 속도, 이동 방향, 공중 여부로 계산 |
| 행동/이동 제어 | Attacking, Dodging, FacingLocked, MovementBlocked | Ability와 Gameplay Tag |
| 지속 게임플레이 | Dead, Stunned, Invulnerable, 버프/디버프 | Gameplay Effect와 Gameplay Tag |

우선 사용할 태그는 다음과 같다.

- `State.Action.Attacking`
- `State.Movement.FacingLocked`
- `State.Movement.Blocked`

이 상태들은 동시에 존재할 수 있으므로 `CurrentStateGEHandle` 하나로 모든 상태를 상호 배타적으로 교체하지 않는다. `CurrentStateGEHandle`을 유지해야 한다면 정말로 단일 상태 슬롯인 별도 개념에만 한정하고, 공격·이동 제어·피격·버프 상태에는 각각의 Ability/Gameplay Effect 수명과 태그 집계를 사용한다.

### 멀티플레이 권한

- 처음부터 서버 권한형 게임플레이로 설계한다.
- 이동은 Unreal의 `ACharacter`와 `CharacterMovementComponent` 복제 경로를 사용한다.
- Ability 실행 가능 여부, 비용, 피해, 회복, 상태 결과는 서버가 최종 확정한다.
- ASC와 AttributeSet은 현재 구조처럼 `ALB_PlayerState`가 소유한다.
- `ALB_PlayerCharacter`는 ASC의 Avatar다.
- 클라이언트는 로컬 입력 수집과 표현을 담당하며 게임플레이 결과를 임의로 확정하지 않는다.
- 각 기능은 PIE의 Listen Server + Client 1개 환경에서 검증한다.
- 현 단계에서는 세션, 로비, 매치메이킹, 전용 서버 배포를 구현하지 않는다.

## Architecture

### `ALB_PlayerCharacter`

- X/Y 이동 입력을 실제 `CharacterMovementComponent` 이동으로 변환한다.
- 마지막 유효 수평 입력으로 `ELB_FacingDirection`을 결정하고, `CurrentFacing`을 서버 권한 상태로 복제한다.
- 소유 클라이언트는 방향을 즉시 표현하고 서버 RPC로 전달하며, 다른 클라이언트는 `OnRep_Facing`에서 같은 방향을 적용한다.
- 좌우 방향은 Actor가 아니라 Skeletal Mesh의 Relative Rotation으로 표현한다.
- Mesh 회전 변경 후 `CacheInitialMeshOffset()`을 호출해 네트워크 스무딩이 사용하는 기준 오프셋도 갱신한다.
- 기본 공격 구현 시 Gameplay Tag를 기준으로 방향 잠금과 이동 잠금을 적용할 책임을 가진다.
- `ALB_PlayerState`가 소유한 ASC의 Avatar 역할을 한다.

`CacheInitialMeshOffset()`을 생략하면 `CharacterMovementComponent`의 네트워크 스무딩이 비로컬 캐릭터의 Mesh를 초기 회전으로 되돌린다. 따라서 런타임 Mesh 방향 변경과 초기 오프셋 갱신은 하나의 처리로 유지한다.

### `ALB_PlayerController`

- LB 전용 Enhanced Input Mapping Context를 로컬 플레이어에 등록한다.
- `IA_Move` 등 로컬 입력을 수집해 소유 중인 `ALB_PlayerCharacter`에 전달한다.
- 게임플레이 결과를 로컬에서 확정하지 않는다.

### `ALB_PlayerState`

- replicated ASC와 AttributeSet을 소유한다.
- 리스폰 후에도 유지되어야 하는 플레이어 지속 데이터를 소유한다.
- Ability 실행 결과와 Gameplay Effect 기반 상태의 서버 권한 경로를 유지한다.

### `ULB_GasComponent`

- `ALB_PlayerState`의 ASC와 AttributeSet을 `ALB_PlayerCharacter`에 연결한다.
- 새 ASC를 만들거나 게임플레이 상태의 별도 소유자가 되지 않는다.
- 속성 변경을 이동 및 UI가 구독할 수 있는 이벤트 경로로 연결한다.

### 검증 기준

- [x] Listen Server와 Client 1개가 각각 자신의 캐릭터를 방향키로 움직일 수 있다.
- [x] X 입력이 화면 좌우/주 진행 방향, Y 입력이 화면 깊이 방향으로 적용된다.
- [ ] 대각선 이동 속도가 축 단독 이동 속도보다 빨라지지 않는다.
- [x] +X 입력 후 캐릭터가 +X를 향하고, -X 입력 후 -X를 향한다.
- [x] Y축으로만 이동할 때 직전 좌우 방향이 유지된다.
- [x] 서버와 클라이언트에서 각 캐릭터의 좌우 방향이 동일하게 보인다.
- [ ] `State.Movement.FacingLocked` 중 반대 X 입력을 해도 방향이 바뀌지 않는다.
- [ ] `State.Movement.Blocked` 중 일반 이동이 적용되지 않는다.
- [ ] 첫 기본 공격 중 방향 전환과 일반 이동이 모두 차단된다.
- [ ] Ability 실행 가능 여부와 상태 결과가 서버에서 확정된다.
- [ ] 리스폰 또는 PlayerState 재연결 후 Owner=`PlayerState`, Avatar=`PlayerCharacter`의 GAS Actor Info가 정상이다.

## Trade-offs

- 기존 `CharacterMovementComponent` 복제 경로를 유지해 검증 범위를 줄이는 대신, 벨트스크롤 전용 이동 제약과 좌우 방향 표현은 별도로 구현해야 한다.
- Mesh Relative Rotation으로 좌우를 표현하면 Capsule과 이동 방향을 고정할 수 있지만, 방향을 바꿀 때 네트워크 스무딩의 기준 오프셋도 함께 갱신해야 한다.
- 방향을 사용하는 게임플레이 코드가 `CurrentFacing`에 의존하므로 방향성 컴포넌트와 Actor Forward 사용이 많아지면 Actor 전체를 회전하는 방식의 비용을 다시 비교한다.
- 방향 잠금과 이동 잠금을 Gameplay Tag로 분리하면 Ability별 조합이 가능하지만, 각 Ability가 태그의 부여와 제거 수명을 정확히 관리해야 한다.
- ASC와 AttributeSet을 `ALB_PlayerState`가 소유하면 리스폰 뒤에도 상태를 유지하기 쉽지만, `PossessedBy`와 `OnRep_PlayerState` 양쪽의 Actor Info 초기화가 항상 일관되어야 한다.
- 현재 단계에서 이동과 전투 상태의 확장 지점만 남기고, Mover, Motion Matching, Iris, 범용 로드아웃 같은 선행 추상화는 도입하지 않는다.

## Future

### 구현 순서

- [x] LB 전용 `IA_Move`와 `IMC_Player`를 만든다.
- [x] `ALB_PlayerController`에서 입력을 수집하고 `ALB_PlayerCharacter`의 X/Y 이동으로 연결한다.
- [x] 입력 벡터를 정규화해 대각선 속도를 보정한다.
- [x] 벨트스크롤 고정 카메라를 구성한다.
- [x] 마지막 유효 X 입력으로 정하는 좌우 방향을 복제한다.
- [x] PIE Listen Server + Client 1개 환경에서 이동과 방향을 검증한다.
- [ ] `IA_Attack`을 기본 공격 Ability 활성화 요청까지 연결한다.
- [ ] 기본 공격 Ability 1종을 구현하고 `State.Movement.FacingLocked`와 `State.Movement.Blocked`를 적용한다.

각 단계는 에디터 또는 PIE에서 독립적으로 확인 가능한 수준으로 유지한다.

### 추후 결정 사항

- 이동 영역의 Y축 폭과 경계 처리
- 점프 도입 시 Z축 이동과 깊이 판정의 충돌 규칙
- 공격별 `FacingLocked`/`MovementBlocked` 부여 방식과 Ability 공통 정책
- 입력 버퍼, 콤보 전환, 캔슬 가능 구간
- 루트 모션 또는 이동형 Ability의 서버 권한 처리 방식
- 피격 경직, 넉백, 띄우기와 `CharacterMovementComponent`의 연동 방식
- 근거리 프로토타입 검증 이후 원거리 캐릭터가 공유할 범위
