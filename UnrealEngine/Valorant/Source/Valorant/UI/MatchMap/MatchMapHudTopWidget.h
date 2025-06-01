// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMapHudTopWidget.generated.h"

enum class ERoundSubState : uint8;
struct FKillFeedInfo;
class ABaseAgent;
class AAgentPlayerState;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UValorantGameInstance;
/**
 * 
 */
UCLASS()
class VALORANT_API UMatchMapHudTopWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TObjectPtr<UValorantGameInstance> GameInstance = nullptr;
	
	virtual void NativeConstruct() override;
	
	// 라운드 시작 3초전 카운트다운 음향을 위한 변수
	bool bIsPreRound = false;
	
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockTime = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockBlueScore = nullptr;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlockRedScore = nullptr;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanelTimer = nullptr;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UImage> ImageSpike = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TMap<AAgentPlayerState*, UUserWidget*> PlayerCardMap;
	
	UFUNCTION()
	void UpdateTime(float Time);
	UFUNCTION()
	void UpdateScore(int TeamBlueScore, int TeamRedScore);

	UFUNCTION()
	void OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime);
	UFUNCTION()
	void OnKillEvent(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent, const FKillFeedInfo& Info);
	UFUNCTION(BlueprintImplementableEvent)
	void MarkKillOnPlayerCard(const AAgentPlayerState* VictimPS);
	UFUNCTION(BlueprintImplementableEvent)
	void InitPlayerCard();
};
