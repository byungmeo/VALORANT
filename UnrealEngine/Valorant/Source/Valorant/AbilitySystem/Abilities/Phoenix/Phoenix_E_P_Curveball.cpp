// Fill out your copyright notice in the Description page of Project Settings.


#include "Phoenix_E_P_Curveball.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
APhoenix_E_P_Curveball::APhoenix_E_P_Curveball()
{
	PrimaryActorTick.bCanEverTick = true;

	// 메시 컴포넌트 생성
	CurveballMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CurveballMesh"));
	CurveballMesh->SetupAttachment(GetRootComponent());

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Sphere"));
	if (MeshAsset.Succeeded())
	{
		CurveballMesh->SetStaticMesh(MeshAsset.Object);
		CurveballMesh->SetWorldScale3D(FVector(0.3f));  // 크기 조정
	}

	// 충돌체 크기 조정
	if (Sphere)
	{
		Sphere->SetSphereRadius(15.0f);
	}

	// ProjectileMovement 설정
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = InitialSpeed;
		ProjectileMovement->MaxSpeed = InitialSpeed * 1.5f;  // 곡선을 그릴 때 속도가 증가할 수 있음
		ProjectileMovement->ProjectileGravityScale = 0.3f;
		ProjectileMovement->bRotationFollowsVelocity = false;  // 수동으로 회전 제어
		ProjectileMovement->bShouldBounce = false;
		ProjectileMovement->Bounciness = 0.0f;
		ProjectileMovement->Friction = 0.0f;
		//ProjectileMovement->bShouldBounce = true;
		//ProjectileMovement->Bounciness = 0.5f;
		//ProjectileMovement->Friction = 0.2f;
	}

	// 섬광 설정 (FlashProjectile의 기본값 오버라이드)
	SetLifeSpan(1.5f);
    
	// 피닉스 커브볼 전용 섬광 시간 설정
	MaxBlindDuration = 1.1f;  // 최대 1.1초 완전 실명
	MinBlindDuration = 0.3f;  // 최소 0.3초 완전 실명
	RecoveryDuration = 0.7f;  // 0.7초 회복
	DetonationDelay = 0.6f;   // 0.5초 후 폭발
}

// Called when the game starts or when spawned
void APhoenix_E_P_Curveball::BeginPlay()
{
	Super::BeginPlay();
	
	// 초기 속도 설정
	if (ProjectileMovement)
	{
		FVector InitialVelocity = GetActorForwardVector() * InitialSpeed;
		ProjectileMovement->Velocity = InitialVelocity;
	}
    
	// 최대 공중 시간 후 자동 폭발
	FTimerHandle ExplosionTimer;
	GetWorld()->GetTimerManager().SetTimer(ExplosionTimer, this, &APhoenix_E_P_Curveball::ExplodeFlash, MaxAirTime, false);
}

// Called every frame
void APhoenix_E_P_Curveball::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ProjectileMovement || !ProjectileMovement->IsActive())
		return;

	TimeSinceSpawn += DeltaTime;

	// 회전 애니메이션
	if (CurveballMesh)
	{
		CurveballMesh->AddLocalRotation(SpinRate * DeltaTime);
	}

	// 곡선 궤적 적용
	if (TimeSinceSpawn >= CurveDelay && !bHasStartedCurving)
	{
		bHasStartedCurving = true;
	}

	if (bHasStartedCurving && CurrentCurveTime < MaxCurveTime)
	{
		CurrentCurveTime += DeltaTime;

		// 현재 속도 방향
		FVector CurrentVelocity = ProjectileMovement->Velocity;
		float CurrentSpeed = CurrentVelocity.Size();
		FVector VelocityDirection = CurrentVelocity.GetSafeNormal();

		// 곡선 방향 계산 (오른쪽 또는 왼쪽)
		FVector RightVector = FVector::CrossProduct(VelocityDirection, FVector::UpVector).GetSafeNormal();
		if (!bShouldCurveRight)
		{
			RightVector *= -1.0f;
		}

		// 곡선 강도 계산 (시간에 따라 감소)
		float CurveRatio = 1.0f - (CurrentCurveTime / MaxCurveTime);
		float CurrentCurveStrength = CurveStrength * CurveRatio;

		// 새로운 속도 방향 계산
		FVector CurveForce = RightVector * CurrentCurveStrength * DeltaTime;
		FVector NewVelocity = CurrentVelocity + CurveForce;
        
		// 속도 유지하면서 방향만 변경
		NewVelocity = NewVelocity.GetSafeNormal() * CurrentSpeed;
		ProjectileMovement->Velocity = NewVelocity;

		// 투사체 회전을 속도 방향에 맞춤
		FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(NewVelocity.GetSafeNormal());
		SetActorRotation(NewRotation);
	}
}

void APhoenix_E_P_Curveball::SetCurveDirection(bool bCurveRight)
{
	bShouldCurveRight = bCurveRight;
}
