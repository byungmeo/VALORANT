#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "SageHealCalculation.generated.h"

UCLASS()
class VALORANT_API USageHealCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	USageHealCalculation();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, 
									   FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};