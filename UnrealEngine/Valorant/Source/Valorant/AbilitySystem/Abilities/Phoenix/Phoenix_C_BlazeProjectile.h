#pragma once

#include "AgentAbility/BaseProjectile.h"
#include "Phoenix_C_BlazeProjectile.generated.h"

UENUM(BlueprintType)
enum class EBlazeMovement2Type : uint8
{
    Straight,   // 직선 이동
    Curved      // 커브 이동
};

UCLASS()
class VALORANT_API APhoenix_C_BlazeProjectile : public ABaseProjectile
{
    GENERATED_BODY()

public:
    APhoenix_C_BlazeProjectile();

    // 벽 생성 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Generation")
    TSubclassOf<class APhoenix_C_BlazeWall> WallSegmentClass;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Generation")
    float SegmentSpawnInterval = 0.05f;  // 벽 세그먼트 생성 간격 (더 촘촘하게)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Generation")
    float SegmentLength = 100.0f;  // 각 세그먼트의 길이
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Generation")
    float MaxWallLength = 2000.0f;  // 최대 벽 길이 (20미터)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Generation")
    int32 MaxSegments = 40;  // 최대 세그먼트 개수 (더 많이)
    
    // 이동 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float StraightSpeed = 1500.0f;  // 직선 이동 속도
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float CurvedSpeed = 1200.0f;    // 커브 이동 속도
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
    float CurveStrength = 300.0f;   // 커브 강도

    // 이동 타입 설정
    void SetMovementType(EBlazeMovement2Type Type);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;

private:
    UPROPERTY()
    EBlazeMovement2Type MovementType = EBlazeMovement2Type::Straight;
    
    UPROPERTY()
    TArray<APhoenix_C_BlazeWall*> SpawnedWalls;
    
    FTimerHandle WallSpawnTimer;
    FVector LastWallSpawnLocation;
    FVector StartLocation;  // 시작 위치 저장
    float TotalDistanceTraveled = 0.0f;
    int32 CurrentSegmentCount = 0;
    float CurveAngle = 0.0f;  // 현재 커브 각도
    
    // 벽 세그먼트 생성
    void SpawnWallSegment();
    
    // 커브 이동 처리
    void ApplyCurveMovement(float DeltaTime);
    
    // 투사체 종료
    void StopProjectile();
};