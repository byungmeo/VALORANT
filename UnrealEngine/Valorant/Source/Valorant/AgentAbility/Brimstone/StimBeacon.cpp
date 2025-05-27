// Fill out your copyright notice in the Description page of Project Settings.


#include "StimBeacon.h"

#include "StimBeaconAnim.h"
#include "StimBeaconGround.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"


AStimBeacon::AStimBeacon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	
	Sphere->SetSphereRadius(20.0f);
	Mesh->SetRelativeScale3D(FVector(1.0f));
	
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BeaconMeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Projectiles/Brimstone_Ability_SpeedStim/AB_Sarge_S0_E_Skelmesh.AB_Sarge_S0_E_Skelmesh'"));
	if (BeaconMeshAsset.Succeeded())
	{
		Mesh->SetSkeletalMeshAsset(BeaconMeshAsset.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClass(TEXT("/Script/Engine.AnimBlueprint'/Game/Resource/Props/Projectiles/Brimstone_Ability_SpeedStim/ABP_StimBeacon.ABP_StimBeacon_C'"));
	if (AnimInstanceClass.Succeeded())
	{
		Mesh->SetAnimInstanceClass(AnimInstanceClass.Class);
	}
	
	// StimBeaconGround 클래스 찾기
	static ConstructorHelpers::FClassFinder<AStimBeaconGround> GroundClass(TEXT("/Script/Engine.Blueprint'/Game/Blueprints/AgentAbility/Brimstone/BP_StimBeaconGround.BP_StimBeaconGround_C'"));
	if (GroundClass.Succeeded())
	{
		StimBeaconGroundClass = GroundClass.Class;
	}
	
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;
}

void AStimBeacon::BeginPlay()
{
	Super::BeginPlay();
	AnimInstance = Cast<UStimBeaconAnim>(Mesh->GetAnimInstance());
	if (AnimInstance)
	{
		AnimInstance->OnOutroEnded.AddDynamic(this, &AStimBeacon::OnOutroAnimationEnded);
		AnimInstance->OnDeployEnded.AddDynamic(this, &AStimBeacon::OnDeployAnimationEnded);
	}
}

void AStimBeacon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ProjectileMovement->IsActive())
	{
		// 설치된 후 스르륵 설치되는 애니메이션 연출을 위해 전방 벡터가 날아가는 방향과 일치하도록 정렬
		const FVector ForwardDir = ProjectileMovement->Velocity.GetSafeNormal2D();
		const FRotator NewRot = ForwardDir.Rotation();
		SetActorRotation(FRotator(0, NewRot.Yaw, 0));
	}
}

void AStimBeacon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AStimBeacon, State);
}

void AStimBeacon::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);

	UE_LOG(LogTemp, Warning, TEXT("%hs Called, ImpactNormal: %s"), __FUNCTION__, *ImpactResult.ImpactNormal.ToString());
	if (ImpactResult.ImpactNormal.Z > 0.8f)
	{
		if (AnimInstance)
		{
			State = EStimBeaconState::ESBS_Active;
			AnimInstance->OnDeploy();
		}
		
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->SetActive(false);
	}
}

void AStimBeacon::OnOutroAnimationEnded()
{
	// Ground 제거
	if (SpawnedGround)
	{
		SpawnedGround->Destroy();
	}
	Destroy();
}

void AStimBeacon::OnDeployAnimationEnded()
{
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("%hs Called, Beacon is activated"), __FUNCTION__);
		
		// StimBeaconGround 생성
		if (StimBeaconGroundClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			FVector SpawnLocation = GetActorLocation();
			SpawnLocation.Z -= 50.0f; // 바닥에 정확히 위치하도록 조정
			
			SpawnedGround = GetWorld()->SpawnActor<AStimBeaconGround>(
				StimBeaconGroundClass, 
				SpawnLocation, 
				FRotator::ZeroRotator, 
				SpawnParams
			);
			
			if (SpawnedGround)
			{
				UE_LOG(LogTemp, Warning, TEXT("StimBeacon: Ground 생성 성공"));
				
				// 일정 시간 후 비콘 제거 시작
				GetWorld()->GetTimerManager().SetTimer(
					UnequipTimerHandle, 
					this, 
					&AStimBeacon::StartUnequip, 
					BuffDuration - UnequipTime, 
					false
				);
			}
		}
	}
}

void AStimBeacon::StartUnequip()
{
	if (AnimInstance)
	{
		State = EStimBeaconState::ESBS_Outtro;
		//AnimInstance->OnOutro();
	}
}