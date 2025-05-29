// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "NiagaraSystem.h"
#include "KayoKnife.generated.h"

class UKayoKnifeAnim;
class AKayoSuppressionZone;

UENUM(BlueprintType)
enum class EKnifeState : uint8
{
	EKS_Idle,
	EKS_Throw,
	EKS_Active
};

UCLASS()
class VALORANT_API AKayoKnife : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AKayoKnife();

private:
	const bool bAutoActivate = false;
	const float Speed = 4125;
	const float Gravity = 0.7f;
	const bool bShouldBounce = true;
	const float EquipTime = 0.8f;
	const float UnequipTime = 0.5f;
	const float SuppressionDuration = 8.0f;
	const float ActiveTime = 1.0f;
	const float SuppressionRadius = 1000.0f; // 10m
	
	FTimerHandle ActiveTimerHandle;
	FVector ImpactLocation;
	FVector ImpactNormal;

public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	EKnifeState State = EKnifeState::EKS_Idle;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UKayoKnifeAnim> AnimInstance = nullptr;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool bIsThirdPerson = false;
	
	// 억제 영역 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Suppression")
	TSubclassOf<AKayoSuppressionZone> SuppressionZoneClass;
	
	// 효과
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* ImpactEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* ImpactSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* ActivationEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* ActivationSound;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
	
	// 억제 영역 생성
	void CreateSuppressionZone();
	
	// 활성화 타이머 콜백
	void ActiveSuppressionZone();

public:
	void OnEquip() const;
	void OnThrow();
	
	UFUNCTION(BlueprintCallable)
	void SetIsThirdPerson(const bool bNew);
};