# VALORITHM
[![ValorithmCinematicMovie](https://github.com/user-attachments/assets/0287dcf2-606c-404b-9c36-8f1a5d5eac8f)](https://youtu.be/p3j4HgjLvsg)

## 프로젝트 개요
Riot Games의 FPS 게임 Valorant를 레퍼런스로 재구현한 멀티플레이 게임

## 개발 기간
'25.04.04 ~ 06.04 (2개월)

## 개발 인원
- **Unreal Engine** : 이충헌, 최연택, 김병대, 김희연
- **AI Engineer** : 김형섭, 이성복, 김형후

## 담당 파트
멀티플레이, 총기 시스템, UI

## 핵심 구현 기능
### Steam 세션 기반 Listen Server 매치메이킹
[핵심 소스코드 링크](https://github.com/byungmeo/VALORANT/blob/d87a17fb8360031a6bee6f07741b4aaeac926ced/UnrealEngine/Valorant/Source/Valorant/GameManager/SubsystemSteamManager.cpp#L294-L370)
### 매치 및 라운드 관리
[핵심 소스코드 링크](https://github.com/byungmeo/VALORANT/blob/e85dbcaa13469fadfd2a973fdc2c90f76ffc952a/UnrealEngine/Valorant/Source/Valorant/GameManager/MatchGameMode.cpp#L552-L598)
### 미니맵
[핵심 소스코드 링크](https://github.com/byungmeo/VALORANT/blob/0037157add32956750f5b2afe73b40a23d22660f/UnrealEngine/Valorant/Source/Valorant/Player/Widget/MiniMapWidget.cpp#L141-L248)
### 총기 반동 / 탄 퍼짐
[핵심 소스코드 링크](https://github.com/byungmeo/VALORANT/blob/1e81affeb634987839563192488925d5fb71e9b2/UnrealEngine/Valorant/Source/Valorant/Weapon/BaseWeapon.cpp#L104-L257)
### 히트스캔
[핵심 소스코드 링크](https://github.com/byungmeo/VALORANT/blob/1e81affeb634987839563192488925d5fb71e9b2/UnrealEngine/Valorant/Source/Valorant/Weapon/BaseWeapon.cpp#L259-L397)
