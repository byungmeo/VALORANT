// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseGround.h"
#include "FireGround.generated.h"

UCLASS()
class VALORANT_API AFireGround : public ABaseGround
{
	GENERATED_BODY()

public:
	AFireGround();

protected:
	virtual void BeginPlay() override;
	virtual void ApplyGameEffect() override;
	
	// OnBeginOverlap 오버라이드 추가
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	// Phoenix Hot Hands 실제 수치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phoenix Hot Hands")
	float DamagePerSecond = 60.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phoenix Hot Hands")
	float HealPerSecond = 12.5f;  // 발로란트 실제 수치
	
	// 틱당 실제 적용 값 (계산용)
	float DamagePerTick = 0.0f;
	float HealPerTick = 0.0f;
	
	// Phoenix 자신인지 확인
	bool IsPhoenixSelf(AActor* Actor) const;
	
	// 틱 값 계산
	void CalculateTickValues();
};