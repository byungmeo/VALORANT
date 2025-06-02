// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MatchPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class VALORANT_API AMatchPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FString DisplayName = "None";
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsBlueTeam = true;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsAttacker = false;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerRPC_OnShift();
};
