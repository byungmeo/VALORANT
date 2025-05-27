#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Sage_C_BarrierOrb.generated.h"

class ABarrierOrbActor;
class ABarrierWallActor;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class VALORANT_API USage_C_BarrierOrb : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    USage_C_BarrierOrb();

protected:
    // 장벽 설정
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    TSubclassOf<ABarrierWallActor> BarrierWallClass;
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    TSubclassOf<ABarrierOrbActor> BarrierOrbClass;
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float MaxPlaceDistance = 1000.f;  // 최대 설치 거리
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float BarrierLifespan = 40.f;  // 장벽 지속시간
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float BarrierHealth = 800.f;  // 세그먼트당 체력
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float BarrierBuildTime = 2.0f;  // 장벽 건설 시간
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    float RotationStep = 15.f;  // 회전 각도 단위
    
    UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
    FVector BarrierSegmentSize = FVector(300.f, 30.f, 300.f);  // 세그먼트 크기
    
    // 미리보기 머티리얼
    UPROPERTY(EditDefaultsOnly, Category = "Preview")
    UMaterialInterface* ValidPreviewMaterial;
    
    UPROPERTY(EditDefaultsOnly, Category = "Preview")
    UMaterialInterface* InvalidPreviewMaterial;
    
    // 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* PlaceEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* BuildEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* PlaceSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* BuildSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects") 
    USoundBase* RotateSound;

    // 오버라이드 함수들
    virtual void WaitAbility() override;
    virtual bool OnLeftClickInput() override;
    virtual bool OnRightClickInput() override;
    virtual bool OnRepeatInput() override;  // 스크롤 입력 처리
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
                           const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

    // 장벽 관련 함수들
    UFUNCTION()
    void SpawnBarrierOrb();
    
    UFUNCTION()
    void DestroyBarrierOrb();
    
    UFUNCTION()
    void UpdateBarrierPreview();
    
    UFUNCTION()
    void RotateBarrier(bool bClockwise);
    
    UFUNCTION(BlueprintCallable)
    bool IsValidPlacement(FVector Location, FRotator Rotation);
    
    UFUNCTION(BlueprintCallable)
    FVector GetBarrierPlaceLocation();
    void SpawnBarrierWall(FVector Location, FRotator Rotation);

    UFUNCTION()
    void CreatePreviewActors();
    
    UFUNCTION()
    void DestroyPreviewActors();
    
    UFUNCTION()
    void UpdatePreviewMaterial(bool bValid);

private:
    UPROPERTY()
    ABarrierOrbActor* SpawnedBarrierOrb;
    
    UPROPERTY()
    ABarrierWallActor* PreviewBarrierWall;
    
    UPROPERTY()
    float CurrentRotation = 0.f;
    
    UPROPERTY()
    bool bIsValidPlacement = false;
    
    FTimerHandle PreviewUpdateTimer;
};