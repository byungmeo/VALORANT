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

	// 곡선 관련 설정
	UPROPERTY(EditAnywhere, Category = "Curveball Settings")
	float InitialSpeed = 2500.0f;
	
	// 폭발까지의 최대 시간
	UPROPERTY(EditAnywhere, Category = "Curveball Settings")
	float MaxAirTime = 1.5f;
	
	// 곡선이 시작되기까지의 지연 시간
	UPROPERTY(EditAnywhere, Category = "Curveball Settings")
	float CurveDelay = 0.15f;

	// 최대 곡선 지속 시간
	UPROPERTY(EditAnywhere, Category = "Curveball Settings")
	float MaxCurveTime = 0.8f;

private:
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* CurveballMesh;
	// 상태 변수
	bool bShouldCurveRight = true;  // true: 오른쪽, false: 왼쪽
	float CurrentCurveTime = 0.0f;
	bool bHasStartedCurving = false;
	float TimeSinceSpawn = 0.0f;

	FRotator SpinRate = FRotator(0, 720, 0);

	float CurveStrength = 1200.f;
};
