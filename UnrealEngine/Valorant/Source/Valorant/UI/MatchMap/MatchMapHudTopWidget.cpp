// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapHudTopWidget.h"

#include "Components/TextBlock.h"
#include "GameManager/MatchGameState.h"
#include "Player/AgentPlayerState.h"
#include "GameManager/ValorantGameInstance.h"
#include "Player/AgentPlayerController.h"
#include "Player/Agent/BaseAgent.h"

void UMatchMapHudTopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	GameInstance = UValorantGameInstance::Get(GetWorld());
	auto* GameState = GetWorld()->GetGameState<AMatchGameState>();
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapHudTopWidget::UpdateTime);
	GameState->OnTeamScoreChanged.AddDynamic(this, &UMatchMapHudTopWidget::UpdateScore);
	GameState->OnRoundSubStateChanged.AddDynamic(this, &UMatchMapHudTopWidget::OnRoundSubStateChanged);

	auto* PC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>();
	PC->OnKillEvent_PC.AddDynamic(this, &UMatchMapHudTopWidget::OnKillEvent);
}

void UMatchMapHudTopWidget::UpdateTime(float Time)
{
	const int Minute = static_cast<int>(Time / 60);
	const int Seconds = static_cast<int>(Time) % 60;
	const FString TimeStr = FString::Printf(TEXT("%d:%02d"), Minute, Seconds);
	TextBlockTime->SetText(FText::FromString(TimeStr));
}

void UMatchMapHudTopWidget::UpdateScore(int TeamBlueScore, int TeamRedScore)
{
	TextBlockBlueScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), TeamBlueScore)));
	TextBlockRedScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), TeamRedScore)));
}

void UMatchMapHudTopWidget::OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime)
{
	if (RoundSubState == ERoundSubState::RSS_PreRound || RoundSubState == ERoundSubState::RSS_BuyPhase)
	{
		InitPlayerCard();
	}
}

void UMatchMapHudTopWidget::OnKillEvent(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent, const FKillFeedInfo& Info)
{
	const auto* VictimPS = VictimAgent->GetPlayerState<AAgentPlayerState>();
	MarkKillOnPlayerCard(VictimPS);
}
