#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HealingOrbActor.generated.h"

UCLASS()
class VALORANT_API AHealingOrbActor : public AActor
{
	GENERATED_BODY()

public:
	AHealingOrbActor();

	// 네트워크 복제 설정
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SetIsReplicated(bool bShouldReplicate);

	// 대상 하이라이트 설정
	UFUNCTION(BlueprintCallable, Category = "Healing Orb")
	void SetTargetHighlight(bool bHighlight);

	// 오직 오너만 볼 수 있도록 설정
	UFUNCTION(BlueprintCallable, Category = "Visibility")
	void SetOnlyOwnerSee(bool bNewOnlyOwnerSee);
	
	// 오너는 볼 수 없도록 설정
	UFUNCTION(BlueprintCallable, Category = "Visibility")
	void SetOwnerNoSee(bool bNewOwnerNoSee);

	// 3인칭 오브 전용 - 오너만 제외하고 표시
	UFUNCTION(BlueprintCallable, Category = "Visibility")
	void SetIsOwnerOnly(bool bIsOwnerOnly);

	// 네트워크 상에서 하이라이트 상태 업데이트
	UFUNCTION(NetMulticast, Reliable)
	void UpdateHighlightState(bool bHighlight);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 오브 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* OrbMesh;

	// 오브 이펙트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UNiagaraComponent* OrbEffect;

	// 하이라이트 이펙트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UNiagaraComponent* HighlightEffect;

	// 포인트 라이트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UPointLightComponent* OrbLight;

	// 오브 설정
	UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
	float OrbRotationSpeed = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
	float OrbPulseSpeed = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
	float OrbPulseScale = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
	FLinearColor NormalColor = FLinearColor(0.0f, 1.0f, 0.5f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
	FLinearColor HighlightColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

	// 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	class USoundBase* OrbIdleSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	class USoundBase* OrbHighlightSound;

private:
	UPROPERTY(Replicated)
	bool bIsHighlighted = false;
	
	float CurrentPulseTime = 0.0f;
	FVector BaseScale;
	bool IsOwnerOnly = false;

	UPROPERTY()
	class UAudioComponent* IdleAudioComponent;

	// 가시성 업데이트
	void UpdateVisibility();
};