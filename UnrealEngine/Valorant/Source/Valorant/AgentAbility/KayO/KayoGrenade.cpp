// Fill out your copyright notice in the Description page of Project Settings.

#include "KayoGrenade.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"

AKayoGrenade::AKayoGrenade()
{
	PrimaryActorTick.bCanEverTick = false;

	// 메시 컴포넌트 생성
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	
	// 콜리전 설정
	Sphere->SetSphereRadius(15.0f);
	Mesh->SetRelativeScale3D(FVector(0.15f));

	// 메시 에셋 설정
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GrenadeMeshAsset(TEXT("/Script/Engine.StaticMesh'/Game/Resource/Agent/KayO/Ability/KayO_AbilityC_Fragment/KayoGrenade.KayoGrenade'"));
	if (GrenadeMeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(GrenadeMeshAsset.Object);
	}
	
	// 머티리얼 설정
	static ConstructorHelpers::FObjectFinder<UMaterial> GrenadeMaterial(TEXT("/Script/Engine.Material'/Game/Resource/Agent/KayO/Ability/KayO_AbilityC_Fragment/M_KayoGrenade.M_KayoGrenade'"));
	if (GrenadeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, GrenadeMaterial.Object);
	}

	// 기본 투사체 이동 설정 (오버핸드)
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->Friction = Friction;
}

void AKayoGrenade::BeginPlay()
{
	Super::BeginPlay();
}

void AKayoGrenade::SetThrowType(EKayoGrenadeThrowType ThrowType)
{
	CurrentThrowType = ThrowType;
	
	// 던지기 타입에 따라 투사체 설정 변경
	if (ProjectileMovement)
	{
		switch (ThrowType)
		{
		case EKayoGrenadeThrowType::Underhand:
			// 언더핸드 설정 - 짧은 거리, 느린 속도
			Speed = UnderhandSpeed;
			Gravity = UnderhandGravity;
			Bounciness = UnderhandBounciness;
			Friction = UnderhandFriction;
			
			ProjectileMovement->InitialSpeed = Speed;
			ProjectileMovement->MaxSpeed = Speed;
			ProjectileMovement->ProjectileGravityScale = Gravity;
			ProjectileMovement->Bounciness = Bounciness;
			ProjectileMovement->Friction = Friction;
			
			UE_LOG(LogTemp, Warning, TEXT("KAYO Grenade - Underhand settings applied (Speed: %.0f, Gravity: %.2f)"), Speed, Gravity);
			break;
			
		case EKayoGrenadeThrowType::Overhand:
			// 오버핸드 설정 - 긴 거리, 빠른 속도
			Speed = OverhandSpeed;
			Gravity = OverhandGravity;
			Bounciness = OverhandBounciness;
			Friction = OverhandFriction;
			
			ProjectileMovement->InitialSpeed = Speed;
			ProjectileMovement->MaxSpeed = Speed;
			ProjectileMovement->ProjectileGravityScale = Gravity;
			ProjectileMovement->Bounciness = Bounciness;
			ProjectileMovement->Friction = Friction;
			
			UE_LOG(LogTemp, Warning, TEXT("KAYO Grenade - Overhand settings applied (Speed: %.0f, Gravity: %.2f)"), Speed, Gravity);
			break;
		}

		const FVector LocalDir = (ThrowType == EKayoGrenadeThrowType::Underhand) ? FVector(0.7f, 0.f, 0.3f) : FVector(1.f, 0.f, 0.f);
		const FVector WorldDir = GetActorRotation().RotateVector(LocalDir).GetSafeNormal();
		ProjectileMovement->Velocity = WorldDir * Speed;
	}
}

void AKayoGrenade::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnProjectileBounced(ImpactResult, ImpactVelocity);
	
	UE_LOG(LogTemp, Warning, TEXT("KAYO Grenade bounced - Impact Normal: %s"), *ImpactResult.ImpactNormal.ToString());
	
	// 바닥에 떨어졌는지 확인 (Z축 노멀이 0.8 이상)
	if (ImpactResult.ImpactNormal.Z > 0.8f)
	{
		if (!bIsExploding)
		{
			StartExplosion();
		}
	}
}

void AKayoGrenade::StartExplosion()
{
	if (!HasAuthority() || bIsExploding)
	{
		return;
	}
	
	bIsExploding = true;
	
	// 폭발 중심 위치 저장
	ExplosionCenter = GetActorLocation();
	
	// 투사체 이동 중지
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->SetActive(false);
	
	// 경고 효과 재생 (모든 클라)
	MulticastPlayWarningEffects(ExplosionCenter);
	
	// 첫 폭발은 ActiveTime 후에 시작
	GetWorld()->GetTimerManager().SetTimer(DeterrentTimerHandle, this, &AKayoGrenade::ActiveDeterrent, 
		DeterrentInterval, true, ActiveTime);
	
	UE_LOG(LogTemp, Warning, TEXT("KAYO Grenade - Explosion started at %s"), *ExplosionCenter.ToString());
}

void AKayoGrenade::ActiveDeterrent()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
	{
		return;
	}
	
	--DeterrentCount;
	UE_LOG(LogTemp, Warning, TEXT("KAYO Grenade - Pulse explosion %d/4"), 4 - DeterrentCount);
	
	// 폭발 효과 재생 (모든 클라)
	MulticastPlayExplosionEffects(ExplosionCenter);
	
	// 데미지 적용
	ApplyExplosionDamage();
	
	// 디버그 표시
	//DrawDebugExplosion();
	
	// 모든 폭발이 끝났으면 파괴
	if (DeterrentCount <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(DeterrentTimerHandle);
		Destroy();
	}
}

void AKayoGrenade::MulticastPlayWarningEffects_Implementation(FVector Location)
{
	if (WarningEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WarningEffect, Location);
	}
	if (WarningSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), WarningSound, Location);
	}
}

void AKayoGrenade::MulticastPlayExplosionEffects_Implementation(FVector Location)
{
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, Location);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, Location);
	}
}

void AKayoGrenade::ApplyExplosionDamage()
{
	if (!DamageEffect)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO Grenade - DamageEffect not set!"));
		return;
	}
	
	// 범위 내 모든 에이전트 찾기
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	// 외부 반경으로 오버랩 체크
	GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		ExplosionCenter,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(OuterRadius),
		QueryParams
	);
	
	// 각 에이전트에 데미지 적용
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (ABaseAgent* Agent = Cast<ABaseAgent>(Result.GetActor()))
		{
			// 시야 차단 체크
			FHitResult HitResult;
			FCollisionQueryParams LineTraceParams;
			LineTraceParams.AddIgnoredActor(this);
			LineTraceParams.AddIgnoredActor(Agent);
			
			bool bHasLineOfSight = !GetWorld()->LineTraceSingleByChannel(
				HitResult,
				ExplosionCenter,
				Agent->GetActorLocation(),
				ECC_WorldStatic,
				LineTraceParams
			);
			
			if (bHasLineOfSight)
			{
				// 거리 계산
				float Distance = FVector::Dist(ExplosionCenter, Agent->GetActorLocation());
				float DamageAmount = CalculateDamageByDistance(Distance);
				
				if (DamageAmount > 0.0f)
				{
					Agent->ServerApplyHealthGE(DamageEffect, -DamageAmount, Cast<ABaseAgent>(Owner));
				}
			}
		}
	}
}

float AKayoGrenade::CalculateDamageByDistance(float Distance) const
{
	if (Distance <= InnerRadius)
	{
		// 내부 반경: 최대 데미지
		return MaxDamage;
	}
	else if (Distance <= OuterRadius)
	{
		// 내부와 외부 반경 사이: 거리에 따라 감소
		float Alpha = (Distance - InnerRadius) / (OuterRadius - InnerRadius);
		return FMath::Lerp(MaxDamage, MinDamage, Alpha);
	}
	else
	{
		// 외부 반경 밖: 데미지 없음
		return 0.0f;
	}
}

void AKayoGrenade::DrawDebugExplosion() const
{
#if ENABLE_DRAW_DEBUG
	if (GEngine && GEngine->GetNetMode(GetWorld()) != NM_DedicatedServer)
	{
		// 내부 반경 (빨간색)
		DrawDebugSphere(GetWorld(), ExplosionCenter, InnerRadius, 16, FColor::Red, false, 1.0f);
		
		// 외부 반경 (노란색)
		DrawDebugSphere(GetWorld(), ExplosionCenter, OuterRadius, 24, FColor::Yellow, false, 1.0f);
		
		// 폭발 위치 표시
		DrawDebugBox(GetWorld(), ExplosionCenter, FVector(20.0f), FColor::Orange, false, 1.0f);
		
		// 남은 폭발 횟수 표시
		FString DebugText = FString::Printf(TEXT("Pulse %d/4"), 4 - DeterrentCount);
		DrawDebugString(GetWorld(), ExplosionCenter + FVector(0, 0, 50), DebugText, nullptr, FColor::White, 1.0f);
		
		// 던지기 타입 표시
		FString ThrowTypeText = CurrentThrowType == EKayoGrenadeThrowType::Underhand ? TEXT("Underhand") : TEXT("Overhand");
		DrawDebugString(GetWorld(), ExplosionCenter + FVector(0, 0, 70), ThrowTypeText, nullptr, FColor::Cyan, 1.0f);
	}
#endif
}