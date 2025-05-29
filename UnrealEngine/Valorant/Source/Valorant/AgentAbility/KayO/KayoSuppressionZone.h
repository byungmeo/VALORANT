// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AgentAbility/BaseGround.h"
#include "NiagaraSystem.h"
#include "KayoSuppressionZone.generated.h"

class UGameplayEffect;
class ABaseAgent;

UCLASS()
class VALORANT_API AKayoSuppressionZone : public ABaseGround
{
	GENERATED_BODY()

public:
	AKayoSuppressionZone();

protected:
	virtual void BeginPlay() override;

private:
	// 억제 영역 설정
	const float SuppressionRadius = 1000.0f;  // 10m 반경
	const float SuppressionDuration = 8.0f;   // 8초 지속
	//const float PulseInterval = 0.5f;         // 0.5초마다 펄스 (비활성화)
	
	FTimerHandle SuppressionDelayHandle;

	// 억제된 에이전트 추적
	TMap<ABaseAgent*, FGameplayEffectSpecHandle> SuppressedAgents;
	
	// 억제 효과 적용
	void ApplySuppression(ABaseAgent* Agent);
	void ApplySuppressionToAllAgents(); // 1초 후 범위 내 Agent에 억제 적용
	void UpdateRangeIndicator();

public:
	// 억제 효과
	UPROPERTY(EditDefaultsOnly, Category = "Suppression")
	TSubclassOf<UGameplayEffect> SuppressionEffect;
	
	// 시각 효과
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* ActivationEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* ActivationSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* PulseEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* PulseSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* SuppressionAppliedEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* SuppressionAppliedSound;
	
	// 범위 표시 머티리얼 (동적 변경용)
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UMaterialInterface* RangeIndicatorMaterial;
	
	UPROPERTY()
	class UMaterialInstanceDynamic* DynamicRangeMaterial;
};