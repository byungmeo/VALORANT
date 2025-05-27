#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Brimstone_Q_Incendiary.generated.h"

class ABaseGround;

UCLASS()
class VALORANT_API UBrimstone_Q_Incendiary : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UBrimstone_Q_Incendiary();

protected:
    virtual bool OnLeftClickInput() override;
};