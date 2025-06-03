// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DatabaseManager.generated.h"

USTRUCT(BlueprintType)
struct FPlayerDTO
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString player_id; // PK
	UPROPERTY(BlueprintReadOnly) FString platform;
	UPROPERTY(BlueprintReadOnly) int32 win_count = 0;
	UPROPERTY(BlueprintReadOnly) int32 draw_count = 0;
	UPROPERTY(BlueprintReadOnly) int32 defeat_count = 0;
	UPROPERTY(BlueprintReadOnly) int32 elo = 1000;
	UPROPERTY(BlueprintReadOnly) int32 total_playseconds = 0;
};

USTRUCT(BlueprintType)
struct FMatchDTO
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int match_id = 0; // PK
	UPROPERTY(BlueprintReadOnly) int map_id = 0;
	UPROPERTY(BlueprintReadOnly) FString start_timestamp;
	UPROPERTY(BlueprintReadOnly) FString end_timestamp;
	UPROPERTY(BlueprintReadOnly) int blue_score = 0;
	UPROPERTY(BlueprintReadOnly) int red_score = 0;
};

/*
 *	특정 Player가 특정 Match에서 기록한 전적
 */
USTRUCT(BlueprintType)
struct FPlayerMatchDTO
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString player_id; // PK
	UPROPERTY(BlueprintReadOnly) int match_id = 0; // PK
	UPROPERTY(BlueprintReadOnly) int team = 0; // 0: blue, 1: red
	UPROPERTY(BlueprintReadOnly) int agent_id = 0; // 해당 매치에서 플레이어가 선택한 요원 ID
	UPROPERTY(BlueprintReadOnly) int kill_count = 0;
	UPROPERTY(BlueprintReadOnly) int death_count = 0;
	UPROPERTY(BlueprintReadOnly) int first_kill_count = 0; // 각 라운드에서 가장 먼저 죽인 횟수
	UPROPERTY(BlueprintReadOnly) int first_death_count = 0; // 각 라운드에서 가장 먼저 죽은 횟수
	UPROPERTY(BlueprintReadOnly) int plant_count = 0; // 스파이크 설치 횟수
	UPROPERTY(BlueprintReadOnly) int defuse_count = 0; // 스파이크 해제 횟수
	UPROPERTY(BlueprintReadOnly) int total_damage = 0; // 입힌 데미지 총합
	UPROPERTY(BlueprintReadOnly) int fire_count = 0; // 총을 발사한 횟수
	UPROPERTY(BlueprintReadOnly) int hit_count = 0; // 총 발사해서 적에게 적중한 횟수
	UPROPERTY(BlueprintReadOnly) int headshot_count = 0; // 총 발사해서 헤드샷 한 횟수
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGetPlayerCompleted, const bool, bIsSuccess, const FPlayerDTO&, PlayerDto);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGetMatchCompleted, const bool, bIsSuccess, const FMatchDTO&, MatchDto);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPostMatchCompleted, const bool, bIsSuccess, const FMatchDTO&, CreatedMatchDto);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGetPlayerMatchCompleted, const bool, bIsSuccess, const FPlayerMatchDTO&, PlayerMatchDto);

/**
 * 
 */
UCLASS()
class VALORANT_API UDatabaseManager : public UObject
{
	GENERATED_BODY()
	
	static UDatabaseManager* Singleton;
	
public:
	static FString DatabaseUrl;
	static UDatabaseManager* GetInstance();
	
	/*
	 *	PLAYER
	 */
	void GetPlayer(const FString& PlayerId, const FString& Platform, const FOnGetPlayerCompleted& Callback);
	void PostPlayer(const FString& PlayerId, const FString& Platform);
	void PutPlayer(const FPlayerDTO& PlayerDto);

	/*
	 *	MATCH
	 */
	void GetMatch(int MatchId, const FOnGetMatchCompleted& Callback);
	void PostMatch(const FOnPostMatchCompleted& Callback);
	void PutMatch(const FMatchDTO& MatchDto);

	/*
	 *	PLAYER_MATCH
	 */
	void GetPlayerMatch(const FString& PlayerId, const int MatchId, const FOnGetPlayerMatchCompleted& Callback);
	void PostPlayerMatch(const FPlayerMatchDTO& PlayerMatchDto);
};