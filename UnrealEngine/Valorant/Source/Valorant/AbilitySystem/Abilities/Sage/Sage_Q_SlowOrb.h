#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Sage_Q_SlowOrb.generated.h"

class ASageSlowOrbEquipped;

UCLASS()
class VALORANT_API USage_Q_SlowOrb : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    USage_Q_SlowOrb();

protected:
    // 어빌리티 준비 단계
    virtual void PrepareAbility() override;
    virtual void WaitAbility() override;
    
    // 후속 입력 처리
    virtual bool OnLeftClickInput() override;
    virtual bool OnRightClickInput() override;
    
    // 어빌리티 종료
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    
    // 슬로우 오브 던지기 실행
    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool ThrowSlowOrb();

    // 장착된 슬로우 오브 생성
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void SpawnEquippedSlowOrbs();
    
    // 장착된 슬로우 오브 제거
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void DestroyEquippedSlowOrbs();

    // 장착된 슬로우 오브 클래스
    UPROPERTY(EditDefaultsOnly, Category = "SlowOrb Settings")
    TSubclassOf<ASageSlowOrbEquipped> EquippedSlowOrbClass;

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* ThrowSound;

private:
    // 3인칭용 슬로우 오브 (다른 플레이어가 봄)
    UPROPERTY()
    ASageSlowOrbEquipped* SpawnedSlowOrb3P;
    
    // 1인칭용 슬로우 오브 (자신만 봄)
    UPROPERTY()
    ASageSlowOrbEquipped* SpawnedSlowOrb1P;
};