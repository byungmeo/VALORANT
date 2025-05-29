// Fill out your copyright notice in the Description page of Project Settings.

#include "Flashbang.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

AFlashbang::AFlashbang()
{
	PrimaryActorTick.bCanEverTick = true;

	// 메시 컴포넌트 생성
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());

	// KAYO 플래시뱅 메시 설정
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Projectiles/KAYO_Ability_Flashbang/AB_Grenadier_S0_4_Skelmesh.AB_Grenadier_S0_4_Skelmesh'"));
	if (MeshAsset.Succeeded())
	{
		Mesh->SetSkeletalMesh(MeshAsset.Object);
	}
	
	// 콜리전 설정
	Sphere->SetSphereRadius(20.0f);
	
	// 기본 투사체 설정 (비활성화 상태)
	ProjectileMovement->bAutoActivate = bAutoActivate;
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->Friction = Friction;
	
	// 기본 폭발 시간 설정
	ExplosionTime = MaximumAirTime;
	
	// FlashProjectile의 섬광 설정 (KAYO 전용)
	MaxBlindDuration = 2.25f;      // 최대 2.25초 완전 실명
	MinBlindDuration = 1.5f;        // 최소 1.5초 완전 실명
	RecoveryDuration = 0.5f;        // 0.5초 회복
	FlashRadius = 2000.0f;          // 20m 반경
	DetonationDelay = 0.0f;         // 폭발 지연 없음 (수동 제어)
	
	// 수명 설정 해제 (수동 제어)
	SetLifeSpan(0.0f);
}

void AFlashbang::ActiveProjectileMovement(const bool bAltFire)
{
	bIsAltFire = bAltFire;
	
	// 던지기 방향 계산
	FVector LocalDir = bAltFire ? FVector(0.7f, 0.f, 0.3f) : FVector(1.f, 0.f, 0.f);
	FVector WorldDir = GetActorRotation().RotateVector(LocalDir).GetSafeNormal();
	float SpeedToUse = bAltFire ? SpeedAltFire : Speed;
	
	// 투사체 설정
	ProjectileMovement->Velocity = WorldDir * SpeedToUse;
	ProjectileMovement->InitialSpeed = SpeedToUse;
	ProjectileMovement->MaxSpeed = SpeedToUse;
	
	// 던지기 방식에 따른 폭발 시간 설정
	ExplosionTime = bAltFire ? MaximumAirTimeAltFire : MaximumAirTime;
	
	// 우클릭 시 실명 시간 조정
	if (bAltFire)
	{
		MaxBlindDuration = 1.5f;    // 우클릭 최대 1.5초
		MinBlindDuration = 1.0f;    // 우클릭 최소 1.0초
	}
	
	// 투사체 활성화
	ProjectileMovement->Activate();
}

void AFlashbang::BeginPlay()
{
	Super::BeginPlay();
	// FlashProjectile의 자동 타이머를 덮어쓰지 않음
}

void AFlashbang::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 회전 애니메이션
	if (ProjectileMovement->IsActive())
	{
		FRotator RotationPerTick(240.f * DeltaSeconds, 540.f * DeltaSeconds, 480.f * DeltaSeconds);
		Mesh->AddLocalRotation(RotationPerTick);
	}
	
	// 공중 시간 계산
	if (ProjectileMovement->IsActive() && !bElapsedAirTime)
	{
		CurrentAirTime += DeltaSeconds;
		if (CurrentAirTime >= ExplosionTime)
		{
			OnElapsedMaxAirTime();
		}
	}
}

void AFlashbang::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	
	// 바운스 시 폭발 시간 단축
	ExplosionTime = FMath::Min(ExplosionTime, AirTimeOnBounce);
}

void AFlashbang::OnElapsedMaxAirTime()
{
	if (bElapsedAirTime)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("KAYO Flashbang - Max air time elapsed, exploding"));
	bElapsedAirTime = true;
	
	// 섬광 폭발
	ExplodeFlash();
}

void AFlashbang::ExplodeFlash()
{
	if (!HasAuthority())
	{
		Super::ExplodeFlash();
		return;
	}
	
	// KAYO 특수 효과 (필요 시)
	// 예: KAYO 전용 섬광 효과나 사운드
	
	// 기본 FlashProjectile 폭발 처리
	Super::ExplodeFlash();
}