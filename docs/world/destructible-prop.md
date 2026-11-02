# Destructible Prop

> Status: Superseded

## Overview

설계 원본을 [LB-wiki Destructible Prop](../../../LB-wiki/world/destructible-prop.md)으로 통합했다.
2026-10-29 합의한 단발 파괴·다중 드롭·서버 처리 순서는 위키를 따른다.

## Key Decisions

설계 결정은 위키, AI 실행 계약은 [WP-P9D4](../../.workflow/changes/WP-P9D4/package.yaml)에서 관리한다.

## Architecture

단발 파괴 가드 확정→중복 허용 다중 Loot 추첨·생성→BP 표현 순서다. 상세 계약은 원본을 따른다.

## Trade-offs

형제 디렉터리 링크는 로컬 LB/LB-wiki 체크아웃 기준이다. 설계 본문을 중복 관리하지 않는다.

## Future

확정된 설계 변경은 위키와 관련 WP/CT에 함께 반영한다. 구현·빌드·PIE는 미실행이다.
