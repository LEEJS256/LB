# Dungeon Layout

> Status: Superseded

## Overview

설계 원본을 [LB-wiki Dungeon Layout](../../../LB-wiki/world/dungeon-layout.md)으로 통합했다.
2026-10-29 합의한 고정 Layout→확정 배치 목록과 향후 서버 랜덤 생성 확장 방향은 위키를 따른다.

## Key Decisions

설계 결정은 위키, AI 실행 계약은 [WP-D6L2](../../.workflow/changes/WP-D6L2/package.yaml)에서 관리한다.

## Architecture

2026-11-16 기준은 방을 물리적으로 붙이지 않는 Room/Exit 분기 Graph다. 서버 GameMode가 Exit별 과반수 카운트다운과 전원 즉시 전환으로 목적지 방을 확정하고 GameState가 진행 상태를 공유하며 각 World는 현재/선택 방을 두 슬롯에 로드한다. 상세 계약은 원본을 따른다.

## Trade-offs

형제 디렉터리 링크는 로컬 LB/LB-wiki 체크아웃 기준이다. 설계 본문을 두 저장소에서 중복 관리하지 않는다.

## Future

확정된 설계 변경은 위키와 관련 WP/CT에 함께 반영한다. 구현·빌드·PIE는 미실행이다.
