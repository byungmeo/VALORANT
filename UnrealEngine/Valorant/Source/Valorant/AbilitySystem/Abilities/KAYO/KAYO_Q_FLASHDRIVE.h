#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "KAYO_Q_FLASHDRIVE.generated.h"

class AKayoFlashbangEquipped;

UCLASS()
class VALORANT_API UKAYO_Q_FLASHDRIVE : public UBaseGameplayAbility
{
	GENERATED_BODY()

public:
	UKAYO_Q_FLASHDRIVE();

protected:
	// 어빌리티 준비 단계
	virtual void PrepareAbility() override;
	virtual void WaitAbility() override;
	
	// 후속 입력 처리 (좌클릭: 직선, 우클릭: 포물선)
	virtual bool OnLeftClickInput() override;
	virtual bool OnRightClickInput() override;
	
	// 어빌리티 종료
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	// 플래시뱅 던지기 실행
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool ThrowFlashbang(bool bAltFire);

	// 장착된 플래시뱅 생성
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SpawnEquippedFlashbangs();
	
	// 장착된 플래시뱅 제거
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void DestroyEquippedFlashbangs();

	// 장착된 플래시뱅 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Flashbang Settings")
	TSubclassOf<AKayoFlashbangEquipped> EquippedFlashbangClass;

	// 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	class USoundBase* ThrowSound;

private:
	// 3인칭용 플래시뱅 (다른 플레이어가 봄)
	UPROPERTY()
	AKayoFlashbangEquipped* SpawnedFlashbang3P;
	
	// 1인칭용 플래시뱅 (자신만 봄)
	UPROPERTY()
	AKayoFlashbangEquipped* SpawnedFlashbang1P;
};