// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchResultPage.generated.h"

struct FMatchDTO;
struct FPlayerMatchDTO;
/**
 * 
 */
UCLASS()
class VALORANT_API UMatchResultPage : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void DisplayMatchResult(const FMatchDTO& MatchResult);
	UFUNCTION(BlueprintImplementableEvent)
	void AddRow(const FPlayerMatchDTO& PlayerMatchInfo);
};
