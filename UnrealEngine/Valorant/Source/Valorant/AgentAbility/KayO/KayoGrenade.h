// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "NiagaraSystem.h"
#include "KayoGrenade.generated.h"

class UGameplayEffect;
class ABaseAgent;

UENUM(BlueprintType)
enum class EKayoGrenadeThrowType : uint8
{
	Underhand,    // 언더핸드 (짧은 거리)
	Overhand      // 오버핸드 (긴 거리)
};

UCLASS()
class VALORANT_API AKayoGrenade : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AKayoGrenade();

	// 던지기 타입 설정
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void SetThrowType(EKayoGrenadeThrowType ThrowType);

private:
	// 기본 투사체 설정
	float Speed = 1800;
	float Gravity = 0.3f;
	const bool bShouldBounce = true;
	float Bounciness = 0.2f;
	float Friction = 0.8f;
	
	// 언더핸드 설정
	const float UnderhandSpeed = 1260;      // 70% 속도
	const float UnderhandGravity = 0.5f;   // 더 높은 중력
	const float UnderhandBounciness = 0.15f;
	const float UnderhandFriction = 0.9f;
	
	// 오버핸드 설정
	const float OverhandSpeed = 1800;
	const float OverhandGravity = 0.3f;
	const float OverhandBounciness = 0.2f;
	const float OverhandFriction = 0.8f;
	
	const float EquipTime = 0.7f;
	const float UnequipTime = 0.6f;
	const float ActiveTime = 0.5f;
	
	// 폭발 설정
	const float OuterRadius = 700.0f;  // 7m 외부 최소 데미지
	const float InnerRadius = 100.0f;  // 1m 내부 최대 데미지
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
	
private:
	// 현재 던지기 타입
	EKayoGrenadeThrowType CurrentThrowType = EKayoGrenadeThrowType::Overhand;
};