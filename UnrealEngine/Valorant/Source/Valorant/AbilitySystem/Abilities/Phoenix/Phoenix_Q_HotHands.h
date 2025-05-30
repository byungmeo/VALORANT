#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Phoenix_Q_HotHands.generated.h"

class APhoenixFireOrbActor;

UENUM(BlueprintType)
enum class EPhoenixQThrowType : uint8
{
    None,
    Straight,        // 좌클릭 - 직선 던지기
    Curved          // 우클릭 - 포물선 던지기
};

UCLASS()
class VALORANT_API UPhoenix_Q_HotHands : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UPhoenix_Q_HotHands();

protected:
    // === 오브 설정 ===
    UPROPERTY(EditDefaultsOnly, Category = "Fire Orb")
    TSubclassOf<APhoenixFireOrbActor> FireOrbClass;

    // === 투사체 설정 ===
    UPROPERTY(EditDefaultsOnly, Category = "Projectile Settings")
    TSubclassOf<class ABaseProjectile> StraightProjectileClass;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile Settings")
    TSubclassOf<class ABaseProjectile> CurvedProjectileClass;

    // === 애니메이션 ===
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* ExecutingUnderThrow3pMontage = nullptr;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* ExecutingUnderThrow1pMontage = nullptr;

    // === 사운드 ===
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* ThrowSound;

    // === 오버라이드 함수들 ===
    virtual void WaitAbility() override;
    virtual bool OnLeftClickInput() override;
    virtual bool OnRightClickInput() override;

private:
    // 현재 던지기 타입
    UPROPERTY(BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
    EPhoenixQThrowType CurrentThrowType = EPhoenixQThrowType::None;

    // 오브 관련 함수들
    void SpawnFireOrb();
    void DestroyFireOrb();
    void UpdateOrbThrowType(EPhoenixQThrowType ThrowType);
    
    // 투사체 생성 (타입별)
    bool SpawnProjectileByType(EPhoenixQThrowType ThrowType);
    
    // 어빌리티 종료
    void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                    FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled);

    // 멤버 변수
    // 3인칭용 (다른 플레이어가 봄)
    UPROPERTY()
    APhoenixFireOrbActor* SpawnedFireOrb;

    // 1인칭용 (자신만 봄)
    UPROPERTY()
    APhoenixFireOrbActor* SpawnedFireOrb1P;
};