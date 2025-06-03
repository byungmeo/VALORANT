// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Web/DatabaseManager.h"
#include "TestWidget.generated.h"

class UDatabaseManager;
/**
 * 
 */
UCLASS()
class VALORANT_API UTestWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY()
	UDatabaseManager* DatabaseManager = nullptr;
	FOnGetMatchCompleted OnGetMatchCompletedDelegate;
	FOnPostMatchCompleted OnPostMatchCompletedDelegate;
	FOnGetPlayerMatchCompleted OnGetPlayerMatchCompletedDelegate;

protected:
	virtual void NativeConstruct() override;
	UFUNCTION()
	void OnGetMatchCompleted(const bool bIsSuccess, const FMatchDTO& MatchDto);
	UFUNCTION()
	void OnPostMatchCompleted(const bool bIsSuccess, const FMatchDTO& CreatedMatchDto);
	UFUNCTION()
	void OnGetPlayerMatchCompleted(const bool bIsSuccess, const FPlayerMatchDTO& PlayerMatchDto);
	
public:
	UFUNCTION(BlueprintCallable)
	void GetPlayer();

	UFUNCTION(BlueprintCallable)
	void PostPlayer();

	UFUNCTION(BlueprintCallable)
	void PutPlayer();
	
	UFUNCTION(BlueprintCallable)
	void GetMatch();

	UFUNCTION(BlueprintCallable)
	void PostMatch();
	
	UFUNCTION(BlueprintCallable)
	void PutMatch();

	UFUNCTION(BlueprintCallable)
	void GetPlayerMatch();

	UFUNCTION(BlueprintCallable)
	void PostPlayerMatch();
};
