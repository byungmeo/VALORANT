// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "NiagaraSystem.h"
#include "KayoGrenade.generated.h"

class UGameplayEffect;
class ABaseAgent;

UCLASS()
class VALORANT_API AKayoGrenade : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AKayoGrenade();

private:
	// 투사체 설정
	const float Speed = 1800;
	const float Gravity = 0.3f;
	const bool bShouldBounce = true;
	const float Bounciness = 0.2f;
	const float Friction = 0.8f;
	const float EquipTime = 0.7f;
	const float UnequipTime = 0.6f;
	const float ActiveTime = 0.5f;
	
	// 폭발 설정
	const float InnerRadius = 300.0f;  // 3m 내부 최대 데미지
	const float OuterRadius = 700.0f;  // 7m 외부 최소 데미지
	int32 DeterrentCount = 4;          // 4번의 폭발 펄스
	const float DeterrentInterval = 1.0f;
	const float MinDamage = 25.0f;
	const float MaxDamage = 60.0f;
	
	FTimerHandle DeterrentTimerHandle;
	bool bIsExploding = false;
	
	// 폭발 중심 위치 (바닥에 고정)
	FVector ExplosionCenter;

public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
	
	// 효과
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* ExplosionEffect = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* ExplosionSound = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* WarningEffect = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* WarningSound = nullptr;
	
	// 데미지 GameplayEffect
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

protected:
	virtual void BeginPlay() override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
	
	// 폭발 시작
	void StartExplosion();
	
	// 펄스 폭발 처리
	void ActiveDeterrent();
	
	// 데미지 적용
	void ApplyExplosionDamage();
	
	// 거리에 따른 데미지 계산
	float CalculateDamageByDistance(float Distance) const;
	
	// 디버그 표시
	void DrawDebugExplosion() const;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayExplosionEffects(FVector Location);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayWarningEffects(FVector Location);
};