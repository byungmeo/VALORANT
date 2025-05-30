#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseOrbActor.generated.h"

UENUM(BlueprintType)
enum class EOrbViewType : uint8
{
    FirstPerson,
    ThirdPerson
};

UENUM(BlueprintType)
enum class EOrbAnimationType : uint8
{
    None,
    Float,      // 상하 부유
    Pulse,      // 크기 펄스  
    Custom      // 하위 클래스에서 정의
};

UCLASS(Abstract, BlueprintType)
class VALORANT_API ABaseOrbActor : public AActor
{
    GENERATED_BODY()

public:
    ABaseOrbActor();

    // 오브 타입 설정 (1인칭/3인칭)
    UFUNCTION(BlueprintCallable, Category = "Base Orb")
    EOrbViewType GetOrbViewType();
    void SetOrbViewType(EOrbViewType ViewType);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Owner가 복제될 때 호출
    virtual void OnRep_Owner() override;

    // === 컴포넌트 ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* OrbMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* OrbEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPointLightComponent* OrbLight;

    // === 기본 오브 설정 ===
    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    float OrbRotationSpeed = 60.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    FLinearColor OrbColor = FLinearColor(0.2f, 0.8f, 1.0f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    float OrbScale = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    float LightIntensity = 800.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    float LightRadius = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Orb Settings")
    float EmissiveIntensity = 5.0f;

    // === 애니메이션 설정 ===
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    EOrbAnimationType AnimationType = EOrbAnimationType::None;

    UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (EditCondition = "AnimationType == EOrbAnimationType::Float"))
    float FloatSpeed = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (EditCondition = "AnimationType == EOrbAnimationType::Float"))
    float FloatHeight = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (EditCondition = "AnimationType == EOrbAnimationType::Pulse"))
    float PulseSpeed = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (EditCondition = "AnimationType == EOrbAnimationType::Pulse"))
    float PulseScale = 0.1f;

    // === 사운드 ===
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* OrbIdleSound;

    // === 가상 함수들 ===
    // 하위 클래스에서 커스텀 애니메이션 구현
    virtual void UpdateCustomAnimation(float DeltaTime) {}
    
    // 하위 클래스에서 색상 변경 로직 구현 시 호출
    virtual void UpdateOrbColor(const FLinearColor& NewColor);

    // 하위 클래스에서 추가 가시성 로직 구현
    virtual void OnVisibilityChanged(bool bIsVisible) {}

private:
    // 오브 타입 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_OrbViewType)
    EOrbViewType OrbViewType = EOrbViewType::ThirdPerson;
    
    UFUNCTION()
    void OnRep_OrbViewType();
    
    // 애니메이션 변수들
    float CurrentAnimationTime = 0.0f;
    FVector InitialRelativeLocation;
    FVector BaseScale;

public:
    [[nodiscard]] FVector GetBaseScale() const;

private:
    UPROPERTY()
    class UAudioComponent* IdleAudioComponent;
    
    // 가시성 설정
    void UpdateVisibilitySettings();
    
    // 가시성 초기화 완료 플래그
    bool bVisibilityInitialized = false;

    // 애니메이션 처리
    void UpdateAnimation(float DeltaTime);
    void UpdateFloatAnimation(float DeltaTime);
    void UpdatePulseAnimation(float DeltaTime);
};