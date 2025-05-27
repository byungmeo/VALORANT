#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BarrierOrbActor.generated.h"

UCLASS()
class VALORANT_API ABarrierOrbActor : public AActor
{
    GENERATED_BODY()

public:
    ABarrierOrbActor();

    // 설치 가능 여부 표시
    UFUNCTION(BlueprintCallable, Category = "Barrier Orb")
    void SetPlacementValid(bool bValid);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 오브 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* OrbMesh;

    // 오브 이펙트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* OrbEffect;

    // 포인트 라이트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPointLightComponent* OrbLight;

    // 오브 설정
    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    float OrbRotationSpeed = 60.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    float OrbFloatSpeed = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    float OrbFloatHeight = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    FLinearColor ValidColor = FLinearColor(0.2f, 0.8f, 1.0f, 1.0f);  // 청록색

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    FLinearColor InvalidColor = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);  // 빨간색

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* OrbIdleSound;

private:
    bool bIsPlacementValid = false;
    float CurrentFloatTime = 0.0f;
    FVector InitialRelativeLocation;

    UPROPERTY()
    class UAudioComponent* IdleAudioComponent;
};