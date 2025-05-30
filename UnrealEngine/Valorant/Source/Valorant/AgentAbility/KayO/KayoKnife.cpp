// Fill out your copyright notice in the Description page of Project Settings.

#include "KayoKnife.h"
#include "KayoKnifeAnim.h"
#include "KayoSuppressionZone.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"

AKayoKnife::AKayoKnife()
{
	PrimaryActorTick.bCanEverTick = true;

	// 메시 컴포넌트 생성
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());

	// 메시 에셋 설정
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Projectiles/KAYO_Ability_Knife/AB_Grenadier_S0_E_Knife_Skelmesh.AB_Grenadier_S0_E_Knife_Skelmesh'"));
	if (MeshAsset.Succeeded())
	{
		Mesh->SetSkeletalMesh(MeshAsset.Object);
	}

	// 애니메이션 블루프린트 설정
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClass(TEXT("/Script/Engine.AnimBlueprint'/Game/Resource/Props/Projectiles/KAYO_Ability_Knife/ABP_Knife.ABP_Knife_C'"));
	if (AnimInstanceClass.Succeeded())
	{
		Mesh->SetAnimInstanceClass(AnimInstanceClass.Class);
	}

	// 콜리전 설정
	Sphere->SetSphereRadius(5.0f);
	Mesh->SetRelativeScale3D(FVector(1.0f));
	Mesh->SetRelativeLocation(FVector(-50, 0, 0));
	Mesh->SetRelativeRotation(FRotator(-90, 0, 0));

	// 투사체 설정
	ProjectileMovement->bAutoActivate = bAutoActivate;
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
}

void AKayoKnife::BeginPlay()
{
	Super::BeginPlay();
	AnimInstance = Cast<UKayoKnifeAnim>(Mesh->GetAnimInstance());
}

void AKayoKnife::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 투사체 회전 애니메이션
	if (ProjectileMovement->IsActive())
	{
		const FRotator RotationPerTick(0, 1080.f * DeltaSeconds, 0);
		Mesh->AddLocalRotation(RotationPerTick);
	}
}

void AKayoKnife::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKayoKnife, State);
}

void AKayoKnife::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	
	// 충돌 정보 저장
	ImpactLocation = ImpactResult.Location;
	ImpactNormal = ImpactResult.ImpactNormal;
	
	// 나이프가 박힌 방향으로 회전
	const FVector KnifeForward = -ImpactNormal;
	const FRotator KnifeRotation = KnifeForward.Rotation();
	SetActorRotation(KnifeRotation);
	Mesh->SetRelativeRotation(FRotator(-90, 0, 0));

	// 상태 변경
	State = EKnifeState::EKS_Active;
	
	// 투사체 이동 중지
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->SetActive(false);
	
	// 충돌 효과 재생 (모든 클라)
	MulticastPlayImpactEffects(ImpactLocation, ImpactNormal.Rotation());
	
	// 활성화 타이머 시작
	if (HasAuthority() && !GetWorld()->GetTimerManager().IsTimerActive(ActiveTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(ActiveTimerHandle, this, &AKayoKnife::ActiveSuppressionZone, ActiveTime, false);
		
		UE_LOG(LogTemp, Warning, TEXT("KAYO Knife - Impact at %s, activating in %.1f seconds"), 
			*ImpactLocation.ToString(), ActiveTime);
	}
}

void AKayoKnife::ActiveSuppressionZone()
{
	if (!HasAuthority())
	{
		return;
	}
	
	// 활성화 효과 재생 (모든 클라)
	MulticastPlayActivationEffects(GetActorLocation());
	
	// 억제 영역 생성
	CreateSuppressionZone();
	
	// 나이프 파괴
	Destroy();
}

void AKayoKnife::MulticastPlayImpactEffects_Implementation(const FVector& Location, const FRotator& Rotation)
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, Location, Rotation);
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, Location);
	}
}

void AKayoKnife::MulticastPlayActivationEffects_Implementation(const FVector& Location)
{
	if (ActivationEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ActivationEffect, Location);
	}
	if (ActivationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ActivationSound, Location);
	}
}

void AKayoKnife::CreateSuppressionZone()
{
	if (!HasAuthority() || !SuppressionZoneClass)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO Knife - Cannot create suppression zone (Authority: %s, Class: %s)"), 
			HasAuthority() ? TEXT("Yes") : TEXT("No"), 
			SuppressionZoneClass ? TEXT("Valid") : TEXT("Invalid"));
		return;
	}
	
	// 억제 영역 스폰 위치 계산 (바닥에 생성)
	FVector SpawnLocation = ImpactLocation;
	
	// 바닥에 레이캐스트하여 정확한 위치 찾기
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	if (GetWorld()->LineTraceSingleByChannel(GroundHit, SpawnLocation, SpawnLocation - FVector(0, 0, 500), 
		ECC_WorldStatic, QueryParams))
	{
		SpawnLocation = GroundHit.Location;
	}
	
	// 억제 영역 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	AKayoSuppressionZone* SuppressionZone = GetWorld()->SpawnActor<AKayoSuppressionZone>(
		SuppressionZoneClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	
	if (SuppressionZone)
	{
		UE_LOG(LogTemp, Warning, TEXT("KAYO Knife - Suppression zone created at %s"), *SpawnLocation.ToString());
		
		// 디버그 표시
#if ENABLE_DRAW_DEBUG
		DrawDebugSphere(GetWorld(), SpawnLocation, SuppressionRadius, 32, FColor::Purple, false, SuppressionDuration);
		DrawDebugLine(GetWorld(), ImpactLocation, SpawnLocation, FColor::Purple, false, SuppressionDuration, 0, 2.0f);
#endif
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO Knife - Failed to create suppression zone"));
	}
}

void AKayoKnife::OnEquip() const
{
	if (AnimInstance)
	{
		AnimInstance->OnKnifeEquip();
	}
}

void AKayoKnife::OnThrow()
{
	State = EKnifeState::EKS_Throw;
	ProjectileMovement->SetActive(true);
	
	UE_LOG(LogTemp, Warning, TEXT("KAYO Knife - Thrown with velocity %s"), *ProjectileMovement->Velocity.ToString());
}

void AKayoKnife::SetIsThirdPerson(const bool bNew)
{
	bIsThirdPerson = bNew;
}