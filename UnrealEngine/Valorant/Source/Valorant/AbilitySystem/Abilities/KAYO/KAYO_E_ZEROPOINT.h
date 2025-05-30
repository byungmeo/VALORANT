#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "KAYO_E_ZEROPOINT.generated.h"

class AKayoKnifeEquipped;

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
	
	// 어빌리티 종료
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	// 억제 나이프 던지기 실행
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool ThrowKnife();

	// 장착된 나이프 생성
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SpawnEquippedKnives();
	
	// 장착된 나이프 제거
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void DestroyEquippedKnives();

	// 장착된 나이프 위치 업데이트
	UFUNCTION()
	void UpdateKnifePositions();

	// 장착된 나이프 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Knife Settings")
	TSubclassOf<AKayoKnifeEquipped> EquippedKnifeClass;

private:
	// 3인칭용 나이프 (다른 플레이어가 봄)
	UPROPERTY()
	AKayoKnifeEquipped* SpawnedKnife3P;
	
	// 1인칭용 나이프 (자신만 봄)
	UPROPERTY()
	AKayoKnifeEquipped* SpawnedKnife1P;
	
	FTimerHandle KnifeUpdateTimer;
};