#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceManager/ValorantGameType.h"
#include "SageSlowOrbEquipped.generated.h"

UCLASS()
class VALORANT_API ASageSlowOrbEquipped : public AActor
{
    GENERATED_BODY()

public:
    ASageSlowOrbEquipped();

    // 슬로우 오브 타입 설정 (1인칭/3인칭)
    UFUNCTION(BlueprintCallable, Category = "Sage SlowOrb")
    void SetSlowOrbViewType(EViewType ViewType);

    // 장착 애니메이션
    UFUNCTION(BlueprintCallable, Category = "Sage SlowOrb")
    void OnEquip();

    // 해제 애니메이션
    UFUNCTION(BlueprintCallable, Category = "Sage SlowOrb")
    void OnUnequip();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Owner가 복제될 때 호출
    virtual void OnRep_Owner() override;

    // 슬로우 오브 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* SlowOrbMesh;

    // 슬로우 오브 설정
    UPROPERTY(EditDefaultsOnly, Category = "SlowOrb Settings")
    float OrbFloatSpeed = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SlowOrb Settings")
    float OrbFloatHeight = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SlowOrb Settings")
    float OrbRotationSpeed = 90.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SlowOrb Settings")
    float OrbPulseSpeed = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "SlowOrb Settings")
    float OrbPulseScale = 0.1f;

    // 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* SlowOrbGlowEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* SlowOrbTrailEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* GlowEffectComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* TrailEffectComponent;

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* SlowOrbEquipSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* SlowOrbUnequipSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* SlowOrbIdleSound;

private:
    // 슬로우 오브 타입 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_SlowOrbViewType)
    EViewType SlowOrbViewType = EViewType::ThirdPerson;
    
    UFUNCTION()
    void OnRep_SlowOrbViewType();
    
    // 장착 상태 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_IsEquipped)
    bool bIsEquipped = false;
    
    UFUNCTION()
    void OnRep_IsEquipped();
    
    float CurrentFloatTime = 0.0f;
    float CurrentRotation = 0.0f;
    float CurrentPulseTime = 0.0f;
    FVector BaseScale;
    FVector BaseLocation;

    UPROPERTY()
    class UAudioComponent* IdleAudioComponent;
    
    // 가시성 설정
    void UpdateVisibilitySettings();
    
    // 장착 상태 업데이트
    void UpdateEquipVisuals();
    
    // 가시성 초기화 완료 플래그
    bool bVisibilityInitialized = false;

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayEquipSound();
    
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayUnequipSound();
    
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayIdleSound();
};