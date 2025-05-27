#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "KAYO_C_FRAGMENT.generated.h"

UCLASS()
class VALORANT_API UKAYO_C_FRAGMENT : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UKAYO_C_FRAGMENT();

protected:
    // 어빌리티 실행 (수류탄 던지기)
    virtual void ExecuteAbility() override;
};