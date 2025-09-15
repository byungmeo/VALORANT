// Fill out your copyright notice in the Description page of Project Settings.


#include "MiniMapWidget.h"

#include "Valorant.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Valorant/Player/Agent/BaseAgent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerState.h"

// 위젯이 생성될 때 호출되는 함수
void UMiniMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const auto* MyPC = GetWorld()->GetFirstPlayerController();
	if (nullptr == MyPC)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, MyPC is NULL"), __FUNCTION__);
		RemoveFromParent();
		return;
	}
	
	auto* MyPS = MyPC->GetPlayerState<AAgentPlayerState>();
	if (nullptr == MyPS)
	{
		RemoveFromParent();
		return;
	}
	MyPlayerState = MyPS;

	if (MinimapBackground)
	{
		// 가시성 설정 - 디버깅을 위해 완전 가시로 변경
		MinimapBackground->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("미니맵 배경 가시성 설정: Visible"));
	}

	if (IconContainer)
	{
		IconContainer->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("아이콘 컨테이너 가시성 설정: Visible"));
	}

	// 초기 에이전트 스캔 수행
	ScanPlayer();
	GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &UMiniMapWidget::Scan, ScanInterval, true);
}

void UMiniMapWidget::Scan()
{
	ScanPlayer();
	UpdateAgentIcons(); // 모든 에이전트 아이콘 업데이트 함수 호출
}

// 에이전트 자동 스캔 함수 구현
void UMiniMapWidget::ScanPlayer()
{
	// 월드의 모든 BaseAgent 검색
	TArray<AActor*> FoundPlayers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAgentPlayerState::StaticClass(), FoundPlayers);
    
	// 아직 등록되지 않은 플레이어만 추가
	for (AActor* Actor : FoundPlayers)
	{
		const AAgentPlayerState* Player = Cast<AAgentPlayerState>(Actor);
		AddPlayerToMinimap(Player); // 유효성 검사는 안에서 한다
	}
}

// 미니맵에 에이전트 추가 함수
void UMiniMapWidget::AddPlayerToMinimap(const AAgentPlayerState* Player)
{
	// 에이전트가 유효하고 아직 미니맵에 등록되지 않은 경우만
	if (Player && !PlayerArray.Contains(Player))
	{
		const bool bIsMe = MyPlayerState == Player;
		const bool bSameTeam = MyPlayerState->bIsBlueTeam == Player->bIsBlueTeam;
		
		const auto* MyAgent = MyPlayerState->GetPawn<ABaseAgent>();
		const auto* OtherAgent = Player->GetPawn<ABaseAgent>();
		if (MyAgent && OtherAgent)
		{
			const bool bIsDead = OtherAgent->IsDead();
			EVisibilityState VisState;
			const UTexture2D* IconToUse = nullptr;
			
			if ((bIsMe || bSameTeam || MyAgent->ActorIsInView(OtherAgent)) && false == bIsDead)
			{
				VisState = EVisibilityState::Visible;
				IconToUse = OtherAgent->GetMinimapIcon();
			}
			else
			{
				VisState = EVisibilityState::Hidden;
				IconToUse = nullptr;
			}
			PlayerArray.Add(Player);
			CreateAgentIcon(Player, IconToUse, VisState, bIsMe ? 0 : bSameTeam ? 1 : 2);
		}
		else
		{
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, MyAgent or OtherAgent is nullptr"), __FUNCTION__);
		}
	}
}

// 미니맵에서 에이전트 제거 함수
void UMiniMapWidget::RemovePlayerFromMinimap(AAgentPlayerState* Player)
{
	if (Player)
	{
		PlayerArray.Remove(Player); // 미니맵에 표시될 에이전트 목록에서 제거

		// 해당 에이전트의 아이콘도 AgentIconMap에서 제거
		if (UUserWidget** FoundIcon = AgentIconMap.Find(Player))
		{
			if (IsValid(*FoundIcon))
			{
				(*FoundIcon)->RemoveFromParent();
			}
			AgentIconMap.Remove(Player);
		}
	}
}


// 미니맵 스케일 설정 함수
void UMiniMapWidget::SetMinimapScale(float NewScale)
{
	if (NewScale > 0) // 새 스케일이 양수인 경우만 (0 이하는 유효하지 않음)
	{
		MapScale = NewScale; // 미니맵 스케일 업데이트
	}
}


// 월드 좌표를 미니맵 좌표로 변환하는 메서드
FVector2D UMiniMapWidget::WorldToMinimapPosition(const FVector& WorldLocation) const
{
	// 관찰자 액터
	const auto* MyAgent = MyPlayerState->GetPawn<ABaseAgent>();
	if (nullptr == MyAgent)
	{
		// 기본값으로 미니맵 중앙 반환
		return MinimapCenter;
	}

	// World 좌표를 -> UI(2D) 좌표로 변환한다
	// NormalizedWorldLocation = WorldLocation / WorldMapSize
	// 참고사항 : 월드 좌표(Y-Up)와 UI 좌표(Y-Down)의 좌표계 표현 방식이 다르기 때문에 실제 적용 공식이 다릅니다.
	// 또한, 실제 월드맵의 왼쪽 아래 좌표는 (-8400, -8400)이기 때문에 별도로 8400을 더하는 수식이 추가되었습니다.
	const float NormalizedWorldLocationX = (WorldLocation.Y + (WorldmapSize / 2)) / WorldmapSize;
	const float NormalizedWorldLocationY = 1.f - (WorldLocation.X / WorldmapSize);
	
	// MinimapLocation = NormalizedWorldLocation * 2DMinimapSize
	FVector2D MinimapLocation;
	MinimapLocation.X = NormalizedWorldLocationX * MinimapSize;
	MinimapLocation.Y = NormalizedWorldLocationY * MinimapSize;
	
	// 미니맵의 경계와 화면 경계 사이의 거리를 더한다
    MinimapLocation += ConvertOffset;
	
	return MinimapLocation;
}

// 매치에 참가중인 플레이어들의 아이콘을 미니맵에 표시하는 메서드
void UMiniMapWidget::UpdateAgentIcons()
{
	const auto* MyAgent = MyPlayerState->GetPawn<ABaseAgent>();
	if (nullptr == MyAgent)
	{
		return;
	}

	// 내 팀원 목록 캐싱
	TArray<const ABaseAgent*> TeamAgents;
	for (const auto* PS : PlayerArray)
	{
		const auto* OtherAgent = PS->GetPawn<ABaseAgent>();
		const bool bIsSelf = OtherAgent == MyAgent;
		if (nullptr == OtherAgent || bIsSelf || OtherAgent->IsDead())
		{
			continue;
		}
		
		// 같은 팀일 경우 팀원 목록에 추가
		if (OtherAgent->IsBlueTeam() == MyAgent->IsBlueTeam())
		{
			TeamAgents.Add(OtherAgent);
		}
	}

	// 모든 에이전트 위치 및 아이콘 업데이트
	for (const auto* PS : PlayerArray) // 미니맵에 표시될 모든 에이전트에 대해 반복
	{
		const auto* OtherAgent = PS->GetPawn<ABaseAgent>();
		if (nullptr == OtherAgent)
		{
			continue;
		}

		// 에이전트의 월드 위치 가져오기
		FVector TargetActorLocation = OtherAgent->GetActorLocation();
		// 월드 좌표를 미니맵 좌표로 변환
		FVector2D ConvertedMinimapPosition = WorldToMinimapPosition(TargetActorLocation);
		
		const bool bIsMe = MyAgent == OtherAgent;
		const bool bSameTeam = MyAgent->IsBlueTeam() == OtherAgent->IsBlueTeam();
		const bool bIsDead = OtherAgent->IsDead();

		// 내가 직접 본 경우인지 확인한다 ( Actor->WasRecentlyRendered(0.01f) )
		bool bVisible = MyAgent->ActorIsInView(OtherAgent);

		// 내 팀원이 본 경우라면 시야를 공유한다
		if (!bVisible && !bIsMe && !bSameTeam && !bIsDead) // 적만 체크
		{
			for (const auto* Teammate : TeamAgents)
			{
				if (Teammate->ActorIsInView(OtherAgent))
				{
					bVisible = true;
					break;
				}
			}
		}

		// 시야 상태
		EVisibilityState VisState = EVisibilityState::Hidden;
		// 표시할 아이콘 텍스쳐
		UTexture2D* IconToUse = nullptr;
		if (!bIsDead)
		{
			// 나 자신 또는 아군이거나 팀원 중 누군가의 시야 안에 들어있는 적이라면 미니맵에 표시한다
			if (bIsMe || bSameTeam || bVisible)
			{
				IconToUse = OtherAgent->GetMinimapIcon();
				VisState = EVisibilityState::Visible;
			}
		}

		// 미니맵 내 아이콘 업데이트 (블루프린트에서 구현)
		UpdateAgentIcon(PS, ConvertedMinimapPosition, IconToUse, VisState, bIsMe ? 0 : bSameTeam ? 1 : 2); // 블루프린트에서 구현된 함수 호출하여 UI 업데이트
	}
}