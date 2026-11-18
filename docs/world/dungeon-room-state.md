# Dungeon Room State

> Status: Superseded

## Overview

설계 원본을 [LB-wiki Dungeon Room State](../../../LB-wiki/world/dungeon-room-state.md)로 통합했다.

## Key Decisions

죽은 몬스터와 깨진 오브젝트의 재생성 방지, 생존 몬스터가 있는 방의 Exit 잠금 기준은 원본을 따른다.

## Architecture

AI 실행 계약은 [WP-R8K3](../../.workflow/changes/WP-R8K3/package.yaml)에서 관리한다.

## Trade-offs

이 파일은 게임 저장소의 진입 링크만 보존하며 설계 내용을 중복 관리하지 않는다.

## Future

최신 결정과 구현 상태는 위키 원본과 Workflow Manager 계획에서 갱신한다.
