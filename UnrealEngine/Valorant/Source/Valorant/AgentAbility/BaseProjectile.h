// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class VALORANT_API ABaseProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABaseProjectile();
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere = nullptr;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	UFUNCTION()
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulti_ExplosionEffects(FVector Location);
	UFUNCTION(NetMulticast, Reliable)
	void NetMulti_WarningEffects(FVector Location);

	// 효과
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* ExplosionVFX = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* ExplosionSFX = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* WarningVFX = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class USoundBase* WarningSFX = nullptr;
	
};
