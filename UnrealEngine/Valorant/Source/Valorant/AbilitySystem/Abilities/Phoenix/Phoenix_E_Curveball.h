#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Phoenix_E_Curveball.generated.h"

class AFlashProjectile;

UCLASS()
class VALORANT_API UPhoenix_E_Curveball : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UPhoenix_E_Curveball();

    virtual bool OnLeftClickInput() override;
    virtual bool OnRightClickInput() override;
    
    // 섬광탄 투사체 스폰 (BaseGameplayAbility의 SpawnProjectile 사용)
    UFUNCTION(BlueprintCallable, Category = "Flash")
    bool SpawnFlashProjectile(bool IsRight);

    // 섬광 어빌리티 특화 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings")
    TSubclassOf<AFlashProjectile> FlashProjectileClass;
}; 