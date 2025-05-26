// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/FlashProjectile.h"
#include "Phoenix_E_P_Curveball.generated.h"

UCLASS()
class VALORANT_API APhoenix_E_P_Curveball : public AFlashProjectile
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APhoenix_E_P_Curveball();

	virtual void Tick(float DeltaTime) override;
	void SetCurveDirection(bool bCurveRight);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	bool bShouldCurveRight;
		
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* CurveballMesh;
	float InitialSpeed = 1800.f;
	float MaxAirTime = 2.0f;
	float TimeSinceSpawn = 0.f;
	FRotator SpinRate = FRotator(0, 720, 360);
	float CurveDelay = 0.15f;
	bool bHasStartedCurving = false;
	float CurrentCurveTime = 0.f;
	float MaxCurveTime = 1.2f;
	float CurveStrength = 1200.f;
};
