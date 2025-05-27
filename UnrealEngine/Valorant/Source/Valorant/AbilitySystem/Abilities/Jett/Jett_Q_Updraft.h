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
    float UpdraftStrength = 5000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
    float BrakingDecelerationFalling = 6000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
    float GravityScale = 6.f;
}; 