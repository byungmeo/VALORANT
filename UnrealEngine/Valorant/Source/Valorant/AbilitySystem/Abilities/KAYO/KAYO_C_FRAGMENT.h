#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "KAYO_C_FRAGMENT.generated.h"

class AKayoGrenadeEquipped;

UENUM(BlueprintType)
enum class EGrenadeThrowType : uint8
{
    None,
    Underhand,    // 좌클릭 - 언더핸드 (짧은 거리)
    Overhand      // 우클릭 - 오버핸드 (긴 거리)
};

UCLASS()
class VALORANT_API UKAYO_C_FRAGMENT : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UKAYO_C_FRAGMENT();

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
    
    // 수류탄 던지기 실행
    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool ThrowGrenade(EGrenadeThrowType ThrowType);

    // 장착된 수류탄 생성
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void SpawnEquippedGrenades();
    
    // 장착된 수류탄 제거
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void DestroyEquippedGrenades();

    // 투사체 타입별 생성
    bool SpawnProjectileByType(EGrenadeThrowType ThrowType);

    // 장착된 수류탄 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Grenade Settings")
    TSubclassOf<AKayoGrenadeEquipped> EquippedGrenadeClass;

    // 투사체 설정 - 기본 ProjectileClass만 사용 (언더핸드/오버핸드는 설정으로 구분)

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* ThrowSound;

private:
    // 현재 던지기 타입
    EGrenadeThrowType CurrentThrowType = EGrenadeThrowType::None;
    
    // 3인칭용 수류탄 (다른 플레이어가 봄)
    UPROPERTY()
    AKayoGrenadeEquipped* SpawnedGrenade3P;
    
    // 1인칭용 수류탄 (자신만 봄)
    UPROPERTY()
    AKayoGrenadeEquipped* SpawnedGrenade1P;
};