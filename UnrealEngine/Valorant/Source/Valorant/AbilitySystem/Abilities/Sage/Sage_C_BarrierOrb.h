#pragma once

#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Sage_C_BarrierOrb.generated.h"

class ABarrierOrbActor;
class ABarrierWallActor;
class UNiagaraSystem;
class USoundBase;

UCLASS()
class VALORANT_API USage_C_BarrierOrb : public UBaseGameplayAbility
{
	GENERATED_BODY()

public:
	USage_C_BarrierOrb();

protected:
	// 장벽 설정
	UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
	TSubclassOf<ABarrierWallActor> BarrierWallClass;

	UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
	TSubclassOf<ABarrierOrbActor> BarrierOrbClass;

	UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
	float MaxPlaceDistance = 1000.f; // 최대 설치 거리

	UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
	float BarrierLifespan = 40.f; // 장벽 지속시간

	UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
	float BarrierHealth = 800.f; // 세그먼트당 체력

	UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
	float BarrierBuildTime = 2.0f; // 장벽 건설 시간

	UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
	float RotationStep = 30.f; // 회전 각도 

	UPROPERTY(EditDefaultsOnly, Category = "Barrier Settings")
	FVector BarrierSegmentSize = FVector(300.f, 30.f, 300.f); // 세그먼트 크기

	// 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* PlaceEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	USoundBase* PlaceSound;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	USoundBase* BuildSound;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	USoundBase* RotateSound;

	// 오버라이드 함수들
	virtual void WaitAbility() override;
	virtual bool OnLeftClickInput() override;
	virtual bool OnRightClickInput() override;

private:
	// 장벽 관련 함수들
	void SpawnBarrierOrb();
	void DestroyBarrierOrb();
	void UpdateBarrierPreview();
	void RotateBarrier();
	FVector GetBarrierPlaceLocation();
	void SpawnBarrierWall(FVector Location, FRotator Rotation);
	void CreatePreviewWall();
	void DestroyPreviewWall();
	void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled);

	// 멤버 변수
	// 3인칭용 (다른 플레이어가 봄)
	UPROPERTY()
	ABarrierOrbActor* SpawnedBarrierOrb;

	// 1인칭용 (자신만 봄)
	UPROPERTY()
	ABarrierOrbActor* SpawnedBarrierOrb1P;

	UPROPERTY()
	ABarrierWallActor* PreviewBarrierWall;

	float CurrentRotation = 0.f;
	FTimerHandle PreviewUpdateTimer;
};
