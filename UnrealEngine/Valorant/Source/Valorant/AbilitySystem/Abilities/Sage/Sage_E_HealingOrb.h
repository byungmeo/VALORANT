#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Sage_E_HealingOrb.generated.h"

class AHealingOrbActor;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class VALORANT_API USage_E_HealingOrb : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    USage_E_HealingOrb();

protected:
    // 힐링 설정
    UPROPERTY(EditDefaultsOnly, Category = "Heal Settings")
    float HealAmount = 100.f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Heal Settings")
    float SelfHealAmount = 30.f;  // 자가 힐링은 더 적음
    
    UPROPERTY(EditDefaultsOnly, Category = "Heal Settings")
    float HealDuration = 5.f;  // 5초에 걸쳐 힐링
    
    UPROPERTY(EditDefaultsOnly, Category = "Heal Settings")
    float HealTickInterval = 0.1f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Heal Settings")
    float MaxHealRange = 1200.f;  // 최대 힐링 거리
    
    UPROPERTY(EditDefaultsOnly, Category = "Heal Settings")
    float MaxHealAngle = 30.f;  // 조준 각도 제한
    
    UPROPERTY(EditDefaultsOnly, Category = "Heal Settings")
    float HealingCooldown = 45.f;  // 쿨다운 45초

    // 힐링 오브 액터
    UPROPERTY(EditDefaultsOnly, Category = "Heal Orb")
    TSubclassOf<AHealingOrbActor> HealingOrbActorClass;
    
    // 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* HealingEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* HealingTargetEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* HealingStartSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* HealingLoopSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* HealingEndSound;
    
    // GameplayEffect
    UPROPERTY(EditDefaultsOnly, Category = "GameplayEffect")
    TSubclassOf<UGameplayEffect> AllyHealEffect;
    
    UPROPERTY(EditDefaultsOnly, Category = "GameplayEffect")
    TSubclassOf<UGameplayEffect> SelfHealEffect;

    // 오버라이드 함수들
    virtual void PrepareAbility() override;
    virtual void WaitAbility() override;
    virtual bool OnLeftClickInput() override;
    virtual bool OnRightClickInput() override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
                           const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

    // 힐링 관련 함수들
    UFUNCTION()
    void SpawnHealingOrb();
    
    UFUNCTION()
    void DestroyHealingOrb();
    
    UFUNCTION(BlueprintCallable)
    class ABaseAgent* FindHealableAlly();
    
    UFUNCTION(BlueprintCallable)
    bool IsAllyHealable(class ABaseAgent* Target);
    
    UFUNCTION(BlueprintCallable)
    void ApplyHealing(class ABaseAgent* Target, bool bIsSelfHeal);
    
    UFUNCTION(BlueprintCallable)
    void PlayHealingEffects(class ABaseAgent* Target);
    
    UFUNCTION()
    void UpdateHealingOrbPosition();

private:
    // 3인칭용 (다른 플레이어가 봄)
    UPROPERTY()
    AHealingOrbActor* SpawnedHealingOrb;
    
    // 1인칭용 (자신만 봄)
    UPROPERTY()
    AHealingOrbActor* SpawnedHealingOrb1P;
    
    UPROPERTY()
    class ABaseAgent* CurrentHealTarget;
    
    FTimerHandle OrbUpdateTimer;
    FTimerHandle HealingEffectTimer;
};