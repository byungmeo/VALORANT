#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HealingOrbActor.generated.h"

UENUM(BlueprintType)
enum class EOrbViewType : uint8
{
    FirstPerson,
    ThirdPerson
};

UCLASS()
class VALORANT_API AHealingOrbActor : public AActor
{
	GENERATED_BODY()

public:
	AHealingOrbActor();

	// 대상 하이라이트 설정
	UFUNCTION(BlueprintCallable, Category = "Healing Orb")
	void SetTargetHighlight(bool bHighlight);

	// 오브 타입 설정 (1인칭/3인칭)
	UFUNCTION(BlueprintCallable, Category = "Healing Orb")
	void SetOrbViewType(EOrbViewType ViewType);

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
	// 하이라이트 상태 (복제됨)
	UPROPERTY(ReplicatedUsing = OnRep_IsHighlighted)
	bool bIsHighlighted = false;
	
	UFUNCTION()
	void OnRep_IsHighlighted();
	
	// 오브 타입
	EOrbViewType OrbViewType = EOrbViewType::ThirdPerson;
	
	float CurrentPulseTime = 0.0f;
	FVector BaseScale;

	UPROPERTY()
	class UAudioComponent* IdleAudioComponent;
	
	// 내부 하이라이트 업데이트 함수
	void UpdateHighlightVisuals();
	
	// 가시성 설정
	void UpdateVisibilitySettings();
};