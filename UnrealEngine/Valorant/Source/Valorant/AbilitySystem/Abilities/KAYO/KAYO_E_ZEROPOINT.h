#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "KAYO_E_ZEROPOINT.generated.h"

UCLASS()
class VALORANT_API UKAYO_E_ZEROPOINT : public UBaseGameplayAbility
{
	GENERATED_BODY()

public:
	UKAYO_E_ZEROPOINT();

protected:
	// 어빌리티 준비 단계
	virtual void PrepareAbility() override;
	
	// 후속 입력 처리 (좌클릭으로 나이프 던지기)
	virtual bool OnLeftClickInput() override;
	
	// 억제 나이프 던지기 실행
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool ThrowKnife();
};