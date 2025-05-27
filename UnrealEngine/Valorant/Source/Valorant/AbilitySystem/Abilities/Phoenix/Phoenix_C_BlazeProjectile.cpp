#include "Phoenix_C_BlazeProjectile.h"
#include "Phoenix_C_BlazeWall.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

APhoenix_C_BlazeProjectile::APhoenix_C_BlazeProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // 충돌체는 작게 (보이지 않는 투사체)
    Sphere->SetSphereRadius(5.0f);
    Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // 투사체 이동 설정
    ProjectileMovement->InitialSpeed = StraightSpeed;
    ProjectileMovement->MaxSpeed = StraightSpeed;
    ProjectileMovement->ProjectileGravityScale = 0.0f;  // 중력 없음
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    
    // 투사체는 보이지 않음
    Sphere->SetVisibility(false);
}

void APhoenix_C_BlazeProjectile::BeginPlay()
{
    Super::BeginPlay();
    
    StartLocation = GetActorLocation();
    LastWallSpawnLocation = StartLocation;
    
    // 첫 벽 세그먼트 즉시 생성
    SpawnWallSegment();
    
    // 타이머로 주기적으로 벽 생성
    GetWorld()->GetTimerManager().SetTimer(WallSpawnTimer, this, 
        &APhoenix_C_BlazeProjectile::SpawnWallSegment, SegmentSpawnInterval, true);
}

void APhoenix_C_BlazeProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 커브 이동 처리
    if (MovementType == EBlazeMovement2Type::Curved)
    {
        ApplyCurveMovement(DeltaTime);
    }
    
    // 이동 거리 계산
    FVector CurrentLocation = GetActorLocation();
    if (LastWallSpawnLocation != FVector::ZeroVector)
    {
        float DistanceFromStart = FVector::Dist(CurrentLocation, GetActorLocation());
        TotalDistanceTraveled += ProjectileMovement->Velocity.Size() * DeltaTime;
    }
    
    // 최대 거리 도달 시 중지
    if (TotalDistanceTraveled >= MaxWallLength || CurrentSegmentCount >= MaxSegments)
    {
        StopProjectile();
    }
}

void APhoenix_C_BlazeProjectile::SetMovementType(EBlazeMovement2Type Type)
{
    MovementType = Type;
    
    // 이동 타입에 따라 속도 조정
    float NewSpeed = (Type == EBlazeMovement2Type::Straight) ? StraightSpeed : CurvedSpeed;
    ProjectileMovement->InitialSpeed = NewSpeed;
    ProjectileMovement->MaxSpeed = NewSpeed;
    ProjectileMovement->Velocity = GetActorForwardVector() * NewSpeed;
}

void APhoenix_C_BlazeProjectile::SpawnWallSegment()
{
    if (!WallSegmentClass || CurrentSegmentCount >= MaxSegments)
    {
        StopProjectile();
        return;
    }
    
    FVector SpawnLocation = GetActorLocation();
    FRotator SpawnRotation = GetActorRotation();
    
    // 지면에 맞춰 위치 조정
    FHitResult HitResult;
    FVector TraceStart = SpawnLocation + FVector(0, 0, 100);
    FVector TraceEnd = SpawnLocation - FVector(0, 0, 500);
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(GetOwner());
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, 
        ECollisionChannel::ECC_WorldStatic, QueryParams))
    {
        SpawnLocation = HitResult.Location;
        
        // 지면의 법선에 맞춰 회전 조정
        FVector GroundNormal = HitResult.Normal;
        FVector ForwardVector = SpawnRotation.Vector();
        FVector RightVector = FVector::CrossProduct(GroundNormal, ForwardVector).GetSafeNormal();
        ForwardVector = FVector::CrossProduct(RightVector, GroundNormal).GetSafeNormal();
        SpawnRotation = FRotationMatrix::MakeFromXZ(ForwardVector, GroundNormal).Rotator();
    }
    
    // 벽 세그먼트 생성
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    APhoenix_C_BlazeWall* NewWall = GetWorld()->SpawnActor<APhoenix_C_BlazeWall>(
        WallSegmentClass, SpawnLocation, SpawnRotation, SpawnParams);
        
    if (NewWall)
    {
        // 벽 세그먼트 크기 조정 (연속성을 위해)
        if (NewWall->WallCollision)
        {
            float AdjustedLength = SegmentLength * 1.5f;  // 약간 겹치게 설정
            //NewWall->WallCollision->SetBoxExtent(FVector(NewWall->WallWidth / 2.0f, AdjustedLength / 2.0f, NewWall->WallHeight / 2.0f));
            NewWall->WallCollision->SetBoxExtent(FVector(AdjustedLength / 2.0f, NewWall->WallWidth / 2.0f, NewWall->WallHeight / 2.0f));
            
            // 메시도 같이 조정
            if (NewWall->GroundMesh)
            {
                //NewWall->GroundMesh->SetRelativeScale3D(FVector(NewWall->WallWidth / 100.0f, AdjustedLength / 100.0f, NewWall->WallHeight / 100.0f));
                NewWall->GroundMesh->SetRelativeScale3D(FVector(AdjustedLength / 100.0f,NewWall->WallWidth / 100.0f, NewWall->WallHeight / 100.0f));
                //NewWall->GroundMesh->SetRelativeScale3D(FVector(AdjustedLength / 100.0f, AdjustedLength / 100.0f, AdjustedLength / 100.0f));
            }
        }
        
        SpawnedWalls.Add(NewWall);
        CurrentSegmentCount++;
        LastWallSpawnLocation = SpawnLocation;
        
        // 최대 거리 체크
        if (TotalDistanceTraveled >= MaxWallLength)
        {
            StopProjectile();
        }
    }
}

void APhoenix_C_BlazeProjectile::ApplyCurveMovement(float DeltaTime)
{
    if (!ProjectileMovement)
    {
        return;
    }
    
    // 시간에 따라 커브 각도 증가
    CurveAngle += DeltaTime * 30.0f;  // 초당 30도 회전
    
    // 최대 커브 각도 제한 (90도)
    CurveAngle = FMath::Clamp(CurveAngle, 0.0f, 90.0f);
    
    // 현재 속도 방향
    FVector CurrentVelocity = ProjectileMovement->Velocity;
    float CurrentSpeed = CurrentVelocity.Size();
    
    // 커브 방향 계산 (오른쪽으로 커브)
    FQuat RotationQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(CurveAngle * DeltaTime));
    FVector NewDirection = RotationQuat.RotateVector(CurrentVelocity.GetSafeNormal());
    
    // 새로운 속도 설정
    ProjectileMovement->Velocity = NewDirection * CurrentSpeed;
    
    // 회전도 업데이트
    SetActorRotation(NewDirection.Rotation());
}

void APhoenix_C_BlazeProjectile::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
    // 벽에 부딪히면 중지
    StopProjectile();
}

void APhoenix_C_BlazeProjectile::StopProjectile()
{
    // 타이머 정리
    GetWorld()->GetTimerManager().ClearTimer(WallSpawnTimer);
    
    // 투사체 정지
    ProjectileMovement->StopMovementImmediately();
    
    // 투사체 제거
    Destroy();
}