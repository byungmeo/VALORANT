// Fill out your copyright notice in the Description page of Project Settings.

#include "FireGround.h"
#include "Player/Agent/BaseAgent.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFireGround::AFireGround()
{
	// BaseGround의 기본값 재정의
	Radius = 450.0f;
	Duration = 4.0f;
	DamageRate = 0.25f;  // 0.25초마다 효과 적용 (초당 4회)
	
	// 틱당 값 계산
	CalculateTickValues();
}

void AFireGround::BeginPlay()
{
	// 중요: Super를 호출하기 전에 DamageRate 설정
	DamageRate = 0.25f;
	CalculateTickValues();
	
	// BaseGround의 BeginPlay 호출
	Super::BeginPlay();
	
	// 로그로 실제 적용되는 값 확인
	UE_LOG(LogTemp, Warning, TEXT("FireGround Settings - DamageRate: %f, DamagePerTick: %f, HealPerTick: %f"), 
		DamageRate, DamagePerTick, HealPerTick);
}

void AFireGround::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsActorBeingDestroyed() || !HasAuthority())
	{
		return;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		OverlappedAgents.Add(Agent);
		
		// 첫 에이전트가 들어왔을 때만 타이머 시작
		if (OverlappedAgents.Num() == 1 && !GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))
		{
			// 중요: 여기서 DamageRate(0.25초)로 타이머 설정
			GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this, 
				&AFireGround::ApplyGameEffect, DamageRate, true);
			
			// 즉시 첫 효과 적용
			ApplyGameEffect();
			
			UE_LOG(LogTemp, Warning, TEXT("FireGround Timer Started - Rate: %f seconds"), DamageRate);
		}
	}
}

void AFireGround::ApplyGameEffect()
{
	if (IsActorBeingDestroyed() || !HasAuthority())
	{
		return;
	}
	
	// TSet은 반복 중에 수정될 수 있으므로 복사본 생성
	TArray<ABaseAgent*> AgentsCopy = OverlappedAgents.Array();
	
	for (ABaseAgent* Agent : AgentsCopy)
	{
		if (!IsValid(Agent))
		{
			OverlappedAgents.Remove(Agent);
			continue;
		}
		
		// Phoenix 자신인지 확인
		bool bIsSelf = IsPhoenixSelf(Agent);
		
		if (bIsSelf)
		{
			// Phoenix 자신에게는 힐 적용
			if (GameplayEffect)
			{
				Agent->ServerApplyHealthGE(GameplayEffect, HealPerTick);
				
				#if !UE_BUILD_SHIPPING
				UE_LOG(LogTemp, Verbose, TEXT("Phoenix Hot Hands: Healing self for %f"), HealPerTick);
				#endif
			}
		}
		else
		{
			// 모든 다른 에이전트(아군 포함)에게는 데미지 적용
			if (GameplayEffect)
			{
				Agent->ServerApplyHealthGE(GameplayEffect, -DamagePerTick);
				
				#if !UE_BUILD_SHIPPING
				UE_LOG(LogTemp, Verbose, TEXT("Phoenix Hot Hands: Damaging %s for %f"), 
					*Agent->GetName(), DamagePerTick);
				#endif
			}
		}
	}
}

bool AFireGround::IsPhoenixSelf(AActor* Actor) const
{
	// Instigator와 같은 액터인지 확인
	return Actor == GetInstigator();
}

void AFireGround::CalculateTickValues()
{
	// DamageRate 주기로 적용되는 실제 데미지/힐 값 계산
	DamagePerTick = DamagePerSecond * DamageRate;
	HealPerTick = HealPerSecond * DamageRate;
}