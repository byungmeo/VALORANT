// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseGround.h"
#include "IncendiaryGround.generated.h"

UCLASS()
class VALORANT_API AIncendiaryGround : public ABaseGround
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AIncendiaryGround();

protected:
	virtual void BeginPlay() override;
    
	// BaseGround 오버라이드
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, 
		const FHitResult& SweepResult) override;
    
	// 시각 효과를 위한 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UNiagaraComponent* NiagaraLoop;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UNiagaraComponent* NiagaraExplosion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UAudioComponent* FireAudio;

private:
	// Incendiary 설정값 (발로란트 실제 수치)
	const float Radius = 350.0f; // 3.5미터
	const float Duration = 8.0f; // 8초 지속
	const float DamageRate = 0.25f; // 0.25초마다 데미지 (초당 4틱)
};