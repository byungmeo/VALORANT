// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentAnimInstance.h"

#include "Valorant.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/Agent/BaseAgent.h"

void UAgentAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	OwnerAgent = Cast<ABaseAgent>(GetOwningActor());
	if (OwnerAgent)
	{
		OwnerAgent->OnAgentEquip.AddDynamic(this, &UAgentAnimInstance::OnEquip);
		OwnerAgent->OnAgentFire.AddDynamic(this, &UAgentAnimInstance::OnFire);
		OwnerAgent->OnAgentReload.AddDynamic(this, &UAgentAnimInstance::OnReload);
		OwnerAgent->OnAgentDamaged.AddDynamic(this, &UAgentAnimInstance::OnDamaged);
		OwnerAgent->OnSpikeActive.AddDynamic(this, &UAgentAnimInstance::OnSpikeActive);
		OwnerAgent->OnSpikeCancel.AddDynamic(this, &UAgentAnimInstance::OnSpikeCancel);
		OwnerAgent->OnSpikeDeactive.AddDynamic(this, &UAgentAnimInstance::OnSpikeDeactive);
		OwnerAgent->OnSpikeDefuseFinish.AddDynamic(this, &UAgentAnimInstance::OnSpikeDefuseFinish);
	}
}

void UAgentAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	UpdateState();
	CalcFootstep();
}

void UAgentAnimInstance::UpdateState()
{
	if (OwnerAgent)
	{
		FVector velocity = OwnerAgent->GetVelocity();
		FVector forward = OwnerAgent->GetActorForwardVector();
		Speed = FVector::DotProduct(velocity, forward);
		FVector right = OwnerAgent->GetActorRightVector();
		Direction = FVector::DotProduct(velocity, right);

		Pitch = FRotator::NormalizeAxis(OwnerAgent->ReplicatedControlRotation.Pitch);
		// Yaw = FRotator::NormalizeAxis(OwnerAgent->ReplicatedControlRotation.Yaw);
		// NET_LOG(LogTemp, Warning, TEXT("Pitch : %f, Yaw : %f"), Pitch, Yaw);
		
		bIsCrouch = OwnerAgent->bIsCrouched;
		bIsInAir = OwnerAgent->GetCharacterMovement()->IsFalling();
		bIsDead = OwnerAgent->IsDead();
		InteractorPoseIdx = OwnerAgent->GetPoseIdx();
		
		if (InteractorState != OwnerAgent->GetInteractorState())
		{
			InteractorState = OwnerAgent->GetInteractorState();
			OnChangedWeaponState();
		}
	}
}

void UAgentAnimInstance::CalcFootstep()
{
	if (nullptr == OwnerAgent || nullptr == OwnerAgent->GetCharacterMovement())
	{
		return;
	}

	const FVector& CurrentLocation = OwnerAgent->GetActorLocation();
	const float DeltaDistance = FVector::Dist2D(CurrentLocation, PrevCharacterLocation);
	PrevCharacterLocation = CurrentLocation;

	// 이동 유효성 검사
	const bool bIsFalling = OwnerAgent->GetCharacterMovement()->IsFalling();
	const bool bIsMoving = OwnerAgent->GetCharacterMovement()->Velocity.Size() > 400.f;
	const bool bIsMovingOnGround = OwnerAgent->GetCharacterMovement()->IsMovingOnGround();
	if (false == bIsMovingOnGround || false == bIsMoving || bIsFalling)
	{
		return;
	}

	// 무효인 경우에는 더하면 안되니까 다 체크하고나서 더하는거임
	AccumulatedDistance += DeltaDistance;
	if (AccumulatedDistance >= StepInterval)
	{
		const FFindFloorResult& CurrentFloor = OwnerAgent->GetCharacterMovement()->CurrentFloor;
		const EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(CurrentFloor.HitResult.PhysMaterial.Get());
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, PhysSurfece: %s"), __FUNCTION__, *EnumToString(SurfaceType));
		PlayFootstepSound(SurfaceType, OwnerAgent->GetCharacterMovement()->GetFeetLocation());
		AccumulatedDistance -= StepInterval;
	}
}