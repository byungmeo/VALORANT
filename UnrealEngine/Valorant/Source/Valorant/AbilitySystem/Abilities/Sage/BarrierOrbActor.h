#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BarrierOrbActor.generated.h"

UENUM(BlueprintType)
enum class EBarrierOrbViewType : uint8
{
    FirstPerson,
    ThirdPerson
};

UCLASS()
class VALORANT_API ABarrierOrbActor : public AActor
{
    GENERATED_BODY()

public:
    ABarrierOrbActor();

    // 설치 가능 여부 표시
    UFUNCTION(BlueprintCallable, Category = "Barrier Orb")
    void SetPlacementValid(bool bValid);
    
    // 오브 타입 설정 (1인칭/3인칭)
    UFUNCTION(BlueprintCallable, Category = "Barrier Orb")
    void SetOrbViewType(EBarrierOrbViewType ViewType);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Owner가 복제될 때 호출
    virtual void OnRep_Owner() override;

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
    FLinearColor OrbColor = FLinearColor(0.2f, 0.8f, 1.0f, 1.0f);  // 청록색

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* OrbIdleSound;

private:
    // 설치 가능 상태 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_IsPlacementValid)
    bool bIsPlacementValid = true;
    
    UFUNCTION()
    void OnRep_IsPlacementValid();
    
    // 오브 타입 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_OrbViewType)
    EBarrierOrbViewType OrbViewType = EBarrierOrbViewType::ThirdPerson;
    
    UFUNCTION()
    void OnRep_OrbViewType();
    
    float CurrentFloatTime = 0.0f;
    FVector InitialRelativeLocation;

    UPROPERTY()
    class UAudioComponent* IdleAudioComponent;
    
    // 가시성 설정
    void UpdateVisibilitySettings();
    
    // 설치 가능 여부 업데이트
    void UpdatePlacementVisuals();
    
    // 가시성 초기화 완료 플래그
    bool bVisibilityInitialized = false;
};