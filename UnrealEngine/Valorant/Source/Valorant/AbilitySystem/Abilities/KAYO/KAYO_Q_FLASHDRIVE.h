#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "KAYO_Q_FLASHDRIVE.generated.h"

UCLASS()
class VALORANT_API UKAYO_Q_FLASHDRIVE : public UBaseGameplayAbility
{
	GENERATED_BODY()

public:
	UKAYO_Q_FLASHDRIVE();
	void PrepareAbility();

protected:
	// 후속 입력 처리 (좌클릭: 직선, 우클릭: 포물선)
	virtual bool OnLeftClickInput() override;
	virtual bool OnRightClickInput() override;
	
	// 플래시뱅 던지기 실행
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool ThrowFlashbang(bool bAltFire);
};