#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Jett_Q_Updraft.generated.h"

UCLASS()
class VALORANT_API UJett_Q_Updraft : public UBaseGameplayAbility
{
    GENERATED_BODY()

public:
    UJett_Q_Updraft();
    
    virtual void ExecuteAbility() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
    float UpdraftStrength = 950.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
    float BrakingDecelerationFalling = 800.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
    float GravityScale = 0.75f;
}; 