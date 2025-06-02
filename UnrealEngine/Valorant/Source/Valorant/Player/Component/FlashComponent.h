#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ResourceManager/ValorantGameType.h"
#include "FlashComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlashIntensityChanged, float, NewIntensity, FVector, FlashSourceLocation);

// 섬광 타입 전방선언
enum class EFlashType : uint8;

// 섬광 상태 열거형
UENUM(BlueprintType)
enum class EFlashState : uint8
{
    None,           // 섬광 없음
    CompleteBlind,  // 완전 실명 상태
    Recovery        // 회복 중
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VALORANT_API UFlashComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFlashComponent();

    // 발로란트 스타일 섬광 효과 시작 (완전 실명 → 빠른 회복)
    UFUNCTION(BlueprintCallable, Category = "Flash")
    void FlashEffect(float BlindDuration, float RecoveryDuration, float ViewAngleMultiplier = 1.0f, EFlashType InFlashType = EFlashType::Default, FVector InFlashLocation = FVector::ZeroVector);

    // 섬광 효과 중지
    UFUNCTION(BlueprintCallable, Category = "Flash")
    void StopFlashEffect();

    // 현재 섬광 강도 가져오기
    UFUNCTION(BlueprintCallable, Category = "Flash")
    float GetCurrentFlashIntensity() const { return CurrentFlashIntensity; }

    // 섬광 상태인지 확인
    UFUNCTION(BlueprintCallable, Category = "Flash")
    bool IsFlashed() const { return FlashState != EFlashState::None; }

    // 현재 섬광 상태
    UFUNCTION(BlueprintCallable, Category = "Flash")
    EFlashState GetFlashState() const { return FlashState; }

    // 섬광 타입 가져오기
    UFUNCTION(BlueprintCallable, Category = "Flash")
    EFlashType GetFlashType() const { return CurrentFlashType; }

    // 섬광 위치 가져오기
    UFUNCTION(BlueprintCallable, Category = "Flash")
    FVector GetFlashLocation() const { return FlashLocation; }

    // 섬광 강도 변경 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Flash")
    FOnFlashIntensityChanged OnFlashIntensityChanged;

    // 클라이언트에서 시야 각도 체크 후 섬광 적용
    UFUNCTION(BlueprintCallable, Category = "Flash")
    void CheckViewAngleAndApplyFlash(FVector FlashLocation, float BlindDuration, float RecoveryDuration, EFlashType InFlashType = EFlashType::Default);

    // 시야 각도 계산 (공개 함수로 변경)
    UFUNCTION(BlueprintCallable, Category = "Flash")
    float CalculateViewAngleMultiplier(FVector FlashLocation);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // 섬광 상태 업데이트
    void UpdateFlashEffect();

    // 완전 실명 상태 → 회복 상태로 전환
    UFUNCTION()
    void StartRecoveryPhase();

    // 현재 섬광 상태
    EFlashState FlashState = EFlashState::None;

    // 현재 섬광 강도 (0.0 ~ 1.0)
    float CurrentFlashIntensity = 0.0f;

    // 완전 실명 시간
    float m_BlindDuration = 0.0f;

    // 회복 시간
    float m_RecoveryDuration = 0.0f;

    // 경과 시간
    float ElapsedTime = 0.0f;

    // 완전 실명 → 회복 전환용 타이머
    FTimerHandle BlindToRecoveryTimer;

    // 시야 각도 감소 배율
    float ViewAngleMultiplier = 1.0f;

    // 섬광 타입
    EFlashType CurrentFlashType;

    // 섬광 위치
    FVector FlashLocation;

    // 최소 섬광 효과 적용 여부
    bool bIsMinimumFlash = false;

    // 시야 각도 체크 설정 (FlashProjectile과 동일하게 설정)
    // 90도 이내에 있어야 효과 적용
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float ViewAngleThreshold = 90.0f;

    // 시야각도별 효과 배율 설정
    // Front : 100%
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
    float FrontViewMultiplier = 1.0f;

    // Front Side : 70%
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
    float SideViewMultiplier = 0.7f;

    // Side : 40%
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
    float PeripheralViewMultiplier = 0.4f;

    // 각도 구간 설정
    // 정면 각도 범위
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "90.0"))
    float FrontViewAngle = 30.0f;

    // 측면 각도 범위
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true", ClampMin = "30.0", ClampMax = "90.0"))
    float SideViewAngle = 60.0f;

    // 최소 섬광 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float MinimumFlashDuration = 0.2f;  // 최소 0.2초

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float MinimumFlashIntensity = 0.8f;  // 최소 80% 강도
};