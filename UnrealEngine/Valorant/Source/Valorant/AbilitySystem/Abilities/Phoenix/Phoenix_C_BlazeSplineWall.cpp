#include "Phoenix_C_BlazeSplineWall.h"
#include "Components/SplineMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"

APhoenix_C_BlazeSplineWall::APhoenix_C_BlazeSplineWall()
{
	PrimaryActorTick.bCanEverTick = false;

	// 네트워크 설정
	bReplicates = true;
	SetReplicateMovement(true);

	// 스플라인 컴포넌트
	WallSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WallSpline"));
	SetRootComponent(WallSpline);

	// 기본 메시 설정
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(
		TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	if (DefaultMesh.Succeeded())
	{
		WallMesh = DefaultMesh.Object;
	}

	CalculateTickValues();
}

void APhoenix_C_BlazeSplineWall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void APhoenix_C_BlazeSplineWall::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 서버에서 지속 시간 타이머 설정
		GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle, this,
		                                       &APhoenix_C_BlazeSplineWall::OnElapsedDuration, WallDuration, false);
	}
}

void APhoenix_C_BlazeSplineWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void APhoenix_C_BlazeSplineWall::AddSplinePoint(const FVector& Location)
{
	if (!HasAuthority())
	{
		return;
	}

	// 서버에서 스플라인 포인트 추가
	WallSpline->AddSplinePoint(Location, ESplineCoordinateSpace::World);

	// 클라이언트에 동기화
	Multicast_AddSplinePoint(Location);
}

void APhoenix_C_BlazeSplineWall::Multicast_AddSplinePoint_Implementation(FVector Location)
{
	if (!HasAuthority())
	{
		WallSpline->AddSplinePoint(Location, ESplineCoordinateSpace::World);
	}

	// 실시간으로 스플라인 메시 업데이트
	UpdateSplineMesh();
}

void APhoenix_C_BlazeSplineWall::FinalizeWall()
{
	if (!HasAuthority())
	{
		return;
	}

	// 모든 클라이언트에서 벽 완성
	Multicast_FinalizeWall();
}

void APhoenix_C_BlazeSplineWall::Multicast_FinalizeWall_Implementation()
{
	// 최종 스플라인 메시 업데이트
	WallSpline->UpdateSpline();
	UpdateSplineMesh();

	// 서버에서만 충돌 이벤트 바인딩
	if (HasAuthority())
	{
		for (UBoxComponent* Collision : CollisionComponents)
		{
			if (Collision)
			{
				Collision->OnComponentBeginOverlap.AddDynamic(this, &APhoenix_C_BlazeSplineWall::OnBeginOverlap);
				Collision->OnComponentEndOverlap.AddDynamic(this, &APhoenix_C_BlazeSplineWall::OnEndOverlap);
			}
		}
	}
}

void APhoenix_C_BlazeSplineWall::UpdateSplineMesh()
{
	// 기존 스플라인 메시 제거
	for (USplineMeshComponent* SplineMesh : SplineMeshComponents)
	{
		if (SplineMesh)
		{
			SplineMesh->DestroyComponent();
		}
	}
	SplineMeshComponents.Empty();

	for (UBoxComponent* Collision : CollisionComponents)
	{
		if (Collision)
		{
			Collision->DestroyComponent();
		}
	}
	CollisionComponents.Empty();

	// 새로운 스플라인 메시 생성
	int32 NumPoints = WallSpline->GetNumberOfSplinePoints();
	if (NumPoints < 2)
	{
		return;
	}

	for (int32 i = 0; i < NumPoints - 1; i++)
	{
		// 스플라인 메시 컴포넌트 생성
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this,
		                                                                   USplineMeshComponent::StaticClass(),
		                                                                   FName(*FString::Printf(
			                                                                   TEXT("SplineMesh_%d"), i)));

		SplineMesh->SetStaticMesh(WallMesh);
		for (int index = 0; index < SplineMesh->GetMaterials().Num(); index++)
		{
			SplineMesh->SetMaterial(index, WallMaterial);
		}
		SplineMesh->SetMobility(EComponentMobility::Movable);
		SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMesh->SetupAttachment(WallSpline);
		SplineMesh->RegisterComponent();

		// 스플라인 메시 설정
		FVector StartPos, StartTangent, EndPos, EndTangent;
		WallSpline->GetLocationAndTangentAtSplinePoint(i, StartPos, StartTangent, ESplineCoordinateSpace::Local);
		WallSpline->GetLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent, ESplineCoordinateSpace::Local);

		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

		// 스케일 설정 (두께와 높이)
		SplineMesh->SetStartScale(FVector2D(WallThickness / 100.0f, WallHeight / 100.0f));
		SplineMesh->SetEndScale(FVector2D(WallThickness / 100.0f, WallHeight / 100.0f));

		SplineMeshComponents.Add(SplineMesh);

		SplineMesh->SetForwardAxis(ESplineMeshAxis::X);
		// 충돌 박스 생성 (서버에서만 실제 충돌 처리)
		if (HasAuthority())
		{
			UBoxComponent* CollisionBox = NewObject<UBoxComponent>(this,
			                                                       UBoxComponent::StaticClass(),
			                                                       FName(*FString::Printf(TEXT("Collision_%d"), i)));

			CollisionBox->SetupAttachment(WallSpline);
			CollisionBox->RegisterComponent();

			// 충돌 박스 위치와 크기 설정
			FVector SegmentCenter = (StartPos + EndPos) / 2.0f;
			float SegmentLength = FVector::Dist(StartPos, EndPos);

			CollisionBox->SetRelativeLocation(SegmentCenter);
			CollisionBox->SetBoxExtent(FVector(SegmentLength / 2.0f, WallThickness / 2.0f, WallHeight / 2.0f));

			// 회전 설정
			FVector Direction = (EndPos - StartPos).GetSafeNormal();
			FRotator Rotation = Direction.Rotation();
			CollisionBox->SetRelativeRotation(Rotation);

			CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
			CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

			CollisionComponents.Add(CollisionBox);
		}
	}
}

void APhoenix_C_BlazeSplineWall::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                                const FHitResult& SweepResult)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || !OtherActor || OtherActor == this)
	{
		return;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		if (!OverlappedAgents.Contains(Agent))
		{
			OverlappedAgents.Add(Agent);

			// 첫 번째 에이전트가 들어왔을 때 타이머 시작
			if (OverlappedAgents.Num() == 1 && !GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))
			{
				GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this,
				                                       &APhoenix_C_BlazeSplineWall::ApplyGameEffect,
				                                       EffectApplicationInterval, true);

				// 즉시 첫 효과 적용
				ApplyGameEffect();
			}
		}
	}
}

void APhoenix_C_BlazeSplineWall::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || IsActorBeingDestroyed() || !OtherActor)
	{
		return;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
	{
		OverlappedAgents.Remove(Agent);

		if (OverlappedAgents.Num() == 0)
		{
			GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
		}
	}
}

void APhoenix_C_BlazeSplineWall::ApplyGameEffect()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
	{
		return;
	}

	TArray<ABaseAgent*> AgentsCopy = OverlappedAgents.Array();

	for (ABaseAgent* Agent : AgentsCopy)
	{
		if (!IsValid(Agent))
		{
			OverlappedAgents.Remove(Agent);
			continue;
		}

		Agent->AdjustFlashEffectDirect(0.25f, 0.25f);

		bool bIsPhoenix = IsPhoenixOrAlly(Agent);

		if (bIsPhoenix)
		{
			if (GameplayEffect)
			{
				Agent->ServerApplyHealthGE(GameplayEffect, HealPerTick);
			}
		}
		else
		{
			if (GameplayEffect)
			{
				Agent->ServerApplyHealthGE(GameplayEffect, -DamagePerTick);
			}
		}
	}
}

bool APhoenix_C_BlazeSplineWall::IsPhoenixOrAlly(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (Actor == GetInstigator())
	{
		return true;
	}

	if (ABaseAgent* Agent = Cast<ABaseAgent>(Actor))
	{
		if (ABaseAgent* InstigatorAgent = Cast<ABaseAgent>(GetInstigator()))
		{
			return InstigatorAgent->IsBlueTeam() == Agent->IsBlueTeam();
		}
	}

	return false;
}

void APhoenix_C_BlazeSplineWall::OnElapsedDuration()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);

	SetLifeSpan(0.1f);
}

void APhoenix_C_BlazeSplineWall::CalculateTickValues()
{
	DamagePerTick = DamagePerSecond * EffectApplicationInterval;
	HealPerTick = HealPerSecond * EffectApplicationInterval;
}
