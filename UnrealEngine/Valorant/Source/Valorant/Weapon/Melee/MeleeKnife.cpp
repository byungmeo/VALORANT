// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeKnife.h"

#include "Valorant.h"
#include "Engine/OverlapResult.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/Agent/BaseAgent.h"
#include "Player/Animaiton/AgentAnimInstance.h"


// Sets default values
AMeleeKnife::AMeleeKnife()
{
	PrimaryActorTick.bCanEverTick = true;
	InteractorType = EInteractorType::Melee;
}

void AMeleeKnife::BeginPlay()
{
	Super::BeginPlay();
	InteractorType = EInteractorType::Melee;
}

void AMeleeKnife::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMeleeKnife, bIsAttacking);
	DOREPLIFETIME(AMeleeKnife, bIsCombo);
	DOREPLIFETIME(AMeleeKnife, bIsComboTransition);
}

void AMeleeKnife::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AMeleeKnife::ServerOnly_CanAutoPickUp(ABaseAgent* Agent) const
{
	return false;
}

bool AMeleeKnife::ServerOnly_CanDrop() const
{
	return false;
}

void AMeleeKnife::ServerRPC_SetActive_Implementation(bool bActive)
{
	Super::ServerRPC_SetActive_Implementation(bActive);
	ResetCombo();
}

void AMeleeKnife::StartFire()
{
	if (nullptr == OwnerAgent || nullptr == OwnerAgent->GetController())
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, OwnerAgent or Controller is nullptr"), __FUNCTION__);
		return;
	}
	
	// 플레이 중인 애니메이션이 있고, 아직 콤보 상태에 진입 전이면, 입력 무시
	if (bIsAttacking && bIsCombo == false)
	{
		// NET_LOG(LogTemp, Warning, TEXT("콤보 애니메이션 진행 전이라 입력 무시"));
		return;
	}
	
	Fire();
}

void AMeleeKnife::Fire()
{
	// NET_LOG(LogTemp, Warning, TEXT("칼 콤보 상태 = %d."), MagazineAmmo);
	
	FVector Center = FVector();
	FRotator CameraRotation = FRotator();

	APlayerController* PC = Cast<APlayerController>(OwnerAgent->GetController());
	if (PC)
	{
		int32 ViewportX, ViewportY;
		PC->GetViewportSize(ViewportX, ViewportY);

		float ScreenX = ViewportX * 0.5f;
		float ScreenY = ViewportY * 0.5f;

		FVector WorldLocation;
		FVector WorldDirection;
		
		if (PC->DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldLocation, WorldDirection))
		{
			Center = WorldLocation + WorldDirection * 50.f;
			CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
		}
	}

	if (HasAuthority())
	{
		bIsComboTransition = true;
		DamageBox(Center, CameraRotation);
	}
	else
	{
		ServerRPC_DamageBox(Center, CameraRotation);
	}
	
	switch (MagazineAmmo)
	{
	case 3:
		Server_PlayAttackAnim(AM_Fire1_1P, AM_Fire1_3P);
		break;
	case 2:
		Server_PlayAttackAnim(AM_Fire2_1P, AM_Fire2_3P);
		break;
	case 1:
		Server_PlayAttackAnim(AM_Fire3_1P, AM_Fire3_3P);
		break;
	default:
		break;
	}
}

void AMeleeKnife::ServerRPC_DamageBox_Implementation(FVector center, FRotator rot)
{
	bIsComboTransition = true;
	DamageBox(center, rot);
}


void AMeleeKnife::DamageBox(FVector center, FRotator rot)
{
	auto BoxShape = FCollisionShape::MakeBox(FVector(30,30,5));
	TArray<FOverlapResult> Overlaps;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerAgent);

	const bool bOverlap = GetWorld()->OverlapMultiByChannel(Overlaps, center, rot.Quaternion(),ECC_EngineTraceChannel2, BoxShape, Params);
	// DrawDebugBox(GetWorld(),center,FVector(30,30,5),rot.Quaternion(),FColor::Green,false,2.0f);

	if (bOverlap)
	{
		for (auto& OverlapResult : Overlaps)
		{
			AActor* hitActor = OverlapResult.GetActor();
			ABaseAgent* agent = Cast<ABaseAgent>(hitActor);
			if (agent == nullptr)
			{
				return;
			}
			
			if (agent->IsBlueTeam() == OwnerAgent->IsBlueTeam())
			{
				return;
			}

			agent->ServerApplyGE(DamageEffectClass, OwnerAgent);
		}
	}
}

void AMeleeKnife::ResetCombo()
{
	// NET_LOG(LogTemp,Warning,TEXT("콤보 리셋"));
	bIsAttacking = false;
	bIsCombo = false;
	MagazineAmmo = MagazineSize;
}


void AMeleeKnife::OnMontageEnded(UAnimMontage* AnimMontage, bool bInterrupted)
{
	//콤보에 의해 애니메이션이 끊기면
	if (bInterrupted && bIsComboTransition)
	{
		bIsComboTransition = false;
		return;
	}
	
	ResetCombo();
}

void AMeleeKnife::Server_PlayAttackAnim_Implementation(UAnimMontage* anim1P, UAnimMontage* anim3P)
{
	if (!anim1P || !anim3P)
	{
		return;
	}

	bIsAttacking = true;
	Multicast_PlayAttackAnim(anim1P, anim3P);
}

void AMeleeKnife::Multicast_PlayAttackAnim_Implementation(UAnimMontage* anim1P, UAnimMontage* anim3P)
{
	MagazineAmmo--;

	if (OwnerAgent == nullptr)
	{
		return;
	}

	if (OwnerAgent->IsLocallyControlled())
	{
		UAnimInstance* abp = OwnerAgent->GetABP_1P();
		if (abp == nullptr)
		{
			NET_LOG(LogTemp, Error, TEXT("%hs Called, ABP 1p is nullptr"), __FUNCTION__);
		}
		abp->Montage_Play(anim1P, 1.0f);
	}
	
	UAnimInstance* abp3P = OwnerAgent->GetABP_3P();
	if (abp3P == nullptr)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, ABP 3p is nullptr"), __FUNCTION__);
		return;
	}
	
	float Duration = abp3P->Montage_Play(anim3P, 1.0f);
	
	if (Duration > 0.f && OwnerAgent->HasAuthority())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AMeleeKnife::OnMontageEnded);
	
		abp3P->Montage_SetEndDelegate(EndDelegate, anim3P);
	}
}