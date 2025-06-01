#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Phoenix_E_Curveball.generated.h"

class APhoenix_E_EquippedCurveball;
class AFlashProjectile;

UCLASS()
class VALORANT_API UPhoenix_E_Curveball : public UBaseGameplayAbility
{
	GENERATED_BODY()

public:
	UPhoenix_E_Curveball();

	virtual bool OnLeftClickInput() override;
	virtual bool OnRightClickInput() override;
	virtual void PrepareAbility() override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

	// 섬광탄 투사체 스폰 (BaseGameplayAbility의 SpawnProjectile 사용)
	UFUNCTION(BlueprintCallable, Category = "Flash")
	bool SpawnFlashProjectile(bool IsRight);

	// 섬광 어빌리티 특화 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings")
	TSubclassOf<AFlashProjectile> FlashProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Curveball")
	TSubclassOf<APhoenix_E_EquippedCurveball> EquippedCurveballClass;

private:
	UPROPERTY()
	APhoenix_E_EquippedCurveball* SpawnedCurveball1P;
	UPROPERTY()
	APhoenix_E_EquippedCurveball* SpawnedCurveball3P;

	void SpawnEquippedCurveballs();
	void DestroyEquippedCurveballs();
};
