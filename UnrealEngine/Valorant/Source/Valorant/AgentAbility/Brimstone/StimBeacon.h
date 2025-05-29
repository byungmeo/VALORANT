// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "StimBeacon.generated.h"

class UStimBeaconAnim;
class AStimBeaconGround;

UENUM(BlueprintType)
enum class EStimBeaconState : uint8
{
	ESBS_Idle,
	ESBS_Active,
	ESBS_Outtro
};

UCLASS()
class VALORANT_API AStimBeacon : public ABaseProjectile
{
	GENERATED_BODY()

public:
	AStimBeacon();

private:
	const float Speed = 1650;
	const float Gravity = 0.7f;
	const bool bShouldBounce = true;
	const float Bounciness = 0.5f;
	const float UnequipTime = 0.7f;
	const float Radius = 1200.0f; // 12미터 
	const float BuffDuration = 12.0f; // 12초 지속
	
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
	
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	EStimBeaconState State = EStimBeaconState::ESBS_Idle;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStimBeaconAnim> AnimInstance = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<AStimBeaconGround> StimBeaconGroundClass;
	
	UPROPERTY()
	AStimBeaconGround* SpawnedGround = nullptr;
	
	FTimerHandle UnequipTimerHandle;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;
	
	UFUNCTION()
	void OnOutroAnimationEnded();
	
	UFUNCTION()
	void OnDeployAnimationEnded();
	
	UFUNCTION()
	void StartUnequip();
};