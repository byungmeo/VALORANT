#pragma once

#include "AgentAbility/BaseGround.h"
#include "Phoenix_C_BlazeWall.generated.h"

class UBoxComponent;

UCLASS()
class VALORANT_API APhoenix_C_BlazeWall : public ABaseGround
{
    GENERATED_BODY()

public:
    APhoenix_C_BlazeWall();

    // 벽 설정값들
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallHeight = 300.0f;  // Z축: 벽 높이 (위아래)

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config") 
    float WallThickness = 50.0f;    // X축: 벽 두께 (앞뒤)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallSegmentLength = 100.0f; // Y축: 각 세그먼트 길이 (좌우)
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallWidth = 5.0f;   // 벽 너비
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallDuration = 8.0f;  // 벽 지속 시간
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float DamagePerSecond = 30.0f;  // 초당 데미지
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float HealPerSecond = 12.5f;    // 초당 힐 (Phoenix 자신에게)
    
    // 힐 GameplayEffect
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    TSubclassOf<UGameplayEffect> HealGameplayEffect = nullptr;
    
    // Phoenix Agent 클래스 (Phoenix 판별용)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    TSubclassOf<class ABaseAgent> PhoenixAgentClass = nullptr;
    
    // 벽 충돌체
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    TObjectPtr<UBoxComponent> WallCollision = nullptr;

protected:
    virtual void BeginPlay() override;
    
    // 벽 세그먼트 초기화
    void InitializeWallSegment();
    
    // BaseGround의 함수들 오버라이드
    virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
    virtual void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;
    virtual void ApplyGameEffect() override;
    virtual void OnElapsedDuration() override;
    
    // Phoenix인지 확인
    bool IsPhoenix(AActor* Actor) const;
    
private:
    // 데미지/힐 적용 주기 (초당 60틱 기준)
    const float EffectApplicationRate = 1.0f / 60.0f;
};