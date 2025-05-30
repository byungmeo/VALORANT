#pragma once

#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Phoenix_C_BlazeSplineWall.generated.h"

class USplineMeshComponent;
class UBoxComponent;
class ABaseAgent;

UCLASS()
class VALORANT_API APhoenix_C_BlazeSplineWall : public AActor
{
    GENERATED_BODY()

public:
    APhoenix_C_BlazeSplineWall();

    // 스플라인 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline")
    USplineComponent* WallSpline;

    // 벽 설정값
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallHeight = 300.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallThickness = 50.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float WallDuration = 8.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float DamagePerSecond = 30.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    float HealPerSecond = 12.5f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    UStaticMesh* WallMesh;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    UMaterialInterface* WallMaterial;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Config")
    TSubclassOf<class UGameplayEffect> GameplayEffect;

    // 스플라인 포인트 추가
    UFUNCTION(BlueprintCallable, Category = "Wall")
    void AddSplinePoint(const FVector& Location);
    
    // 벽 생성 완료
    UFUNCTION(BlueprintCallable, Category = "Wall")
    void FinalizeWall();
    
    // 멀티캐스트 함수들
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_AddSplinePoint(FVector Location);
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_FinalizeWall();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY()
    TArray<USplineMeshComponent*> SplineMeshComponents;
    
    UPROPERTY()
    TArray<UBoxComponent*> CollisionComponents;
    
    UPROPERTY()
    TSet<ABaseAgent*> OverlappedAgents;
    
    FTimerHandle DamageTimerHandle;
    FTimerHandle DurationTimerHandle;
    
    float EffectApplicationInterval = 0.25f;
    float DamagePerTick = -7.5f;
    float HealPerTick = 1.5625f;
    
    // 스플라인 메시 업데이트
    void UpdateSplineMesh();
    
    // 충돌 처리
    UFUNCTION()
    void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
    
    void ApplyGameEffect();
    void OnElapsedDuration();
    bool IsPhoenixOrAlly(AActor* Actor) const;
    void CalculateTickValues();
};