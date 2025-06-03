#include "FlashComponent.h"
#include "Engine/World.h"
#include "Player/Agent/BaseAgent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"

UFlashComponent::UFlashComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    CurrentFlashType = EFlashType::Default;
}

void UFlashComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UFlashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (FlashState != EFlashState::None)
    {
        UpdateFlashEffect();
    }
}

void UFlashComponent::CheckViewAngleAndApplyFlash(FVector InFlashLocation, float BlindDuration, float RecoveryDuration, EFlashType InFlashType)
{
    // 로컬 플레이어만 시야 각도 체크
    ABaseAgent* Owner = Cast<ABaseAgent>(GetOwner());
    if (!Owner || !Owner->IsLocallyControlled())
        return;

    // 시야 각도 계산
    ViewAngleMultiplier = CalculateViewAngleMultiplier(InFlashLocation);
    
    // 시야 각도가 임계값을 넘어도 최소 효과 적용
    if (ViewAngleMultiplier <= 0.1f)
    {
        // 각도로 인해 섬광이 거의 안보이지만, 최소한의 효과는 적용
        bIsMinimumFlash = true;
        FlashEffect(MinimumFlashDuration, MinimumFlashDuration, MinimumFlashIntensity, InFlashType, InFlashLocation);
        return;
    }

    bIsMinimumFlash = false;
    // 정상적인 섬광 적용
    FlashEffect(BlindDuration, RecoveryDuration, ViewAngleMultiplier, InFlashType, InFlashLocation);
}

float UFlashComponent::CalculateViewAngleMultiplier(FVector InFlashLocation)
{
    ABaseAgent* Owner = Cast<ABaseAgent>(GetOwner());
    if (!Owner)
        return 0.0f;

    // 플레이어 카메라 방향 가져오기
    FVector CameraLocation;
    FRotator CameraRotation;
    Owner->GetActorEyesViewPoint(CameraLocation, CameraRotation);
    
    FVector CameraForward = CameraRotation.Vector();
    FVector ToFlash = (InFlashLocation - CameraLocation).GetSafeNormal();
    
    // 내적으로 각도 계산
    float DotProduct = FVector::DotProduct(CameraForward, ToFlash);
    float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
    
    // 각도별 배율 적용
    float Multiplier = 0.0f;
    
    // 뒤돌고 있으면 효과 없음
    if (AngleDegrees > ViewAngleThreshold)
    {
        Multiplier = 0.0f;
    }
    else if (AngleDegrees <= FrontViewAngle)
    {
        // 기본값: 1.0 (100%)
        Multiplier = FrontViewMultiplier;
    }
    else if (AngleDegrees <= SideViewAngle)
    {
        // 기본값: 0.7 (70%)
        Multiplier = SideViewMultiplier;
    }
    else
    {
        // 기본값: 0.4 (40%)
        Multiplier = PeripheralViewMultiplier;
    }
    
    // 디버깅 로그
    UE_LOG(LogTemp, Verbose, TEXT("시야각: %.1f도, 배율: %.2f"), AngleDegrees, Multiplier);
    
    return FMath::Clamp(Multiplier, 0.0f, 1.0f);
}

void UFlashComponent::FlashEffect(float InBlindDuration, float InRecoveryDuration, float InViewAngleMultiplier, EFlashType InFlashType, FVector InFlashLocation)
{
    // 섬광 타입과 위치 저장
    CurrentFlashType = InFlashType;
    FlashLocation = InFlashLocation;
    
    // 시야 각도에 따라 지속 시간 조절
    m_BlindDuration = InBlindDuration * InViewAngleMultiplier;
    m_RecoveryDuration = InRecoveryDuration; // 회복 시간은 고정
    ViewAngleMultiplier = InViewAngleMultiplier;
    
    // 최소 효과 적용 시
    if (bIsMinimumFlash)
    {
        m_BlindDuration = MinimumFlashDuration;
        m_RecoveryDuration = 0.0f; // 최소 효과는 즉시 회복
        CurrentFlashIntensity = MinimumFlashIntensity;
        FlashState = EFlashState::Recovery; // 바로 회복 상태로
        ElapsedTime = 0.0f;
    }
    else if (m_BlindDuration <= 0.1f)
    {
        return;
    }
    else
    {
        // 정상적인 섬광 효과
        FlashState = EFlashState::CompleteBlind;
        CurrentFlashIntensity = 1.0f; // 완전 실명
        ElapsedTime = 0.0f;
        
        // 완전 실명 → 회복 전환 타이머 설정
        GetWorld()->GetTimerManager().SetTimer(BlindToRecoveryTimer, this, &UFlashComponent::StartRecoveryPhase, m_BlindDuration, false);
    }

    // 틱 활성화
    SetComponentTickEnabled(true);

    // 델리게이트 호출 - 섬광 위치 정보도 함께 전달
    OnFlashIntensityChanged.Broadcast(CurrentFlashIntensity, FlashLocation);
    
    UE_LOG(LogTemp, Warning, TEXT("섬광 시작: 완전실명 %.1f초, 회복 %.1f초, 각도배율 %.2f, 타입: %d, 최소효과: %s"), 
           m_BlindDuration, m_RecoveryDuration, ViewAngleMultiplier, (int32)CurrentFlashType, bIsMinimumFlash ? TEXT("Yes") : TEXT("No"));
}

void UFlashComponent::StartRecoveryPhase()
{
    if (FlashState == EFlashState::CompleteBlind)
    {
        FlashState = EFlashState::Recovery;
        ElapsedTime = 0.0f; // 회복 단계 시간 리셋
        
        UE_LOG(LogTemp, Warning, TEXT("섬광 회복 시작"));
    }
}

void UFlashComponent::UpdateFlashEffect()
{
    ElapsedTime += GetWorld()->GetDeltaSeconds();

    if (bIsMinimumFlash)
    {
        // 최소 효과는 빠르게 감소
        float RecoveryRatio = FMath::Clamp(ElapsedTime / MinimumFlashDuration, 0.0f, 1.0f);
        CurrentFlashIntensity = MinimumFlashIntensity * (1.0f - RecoveryRatio);
        
        if (RecoveryRatio >= 1.0f)
        {
            StopFlashEffect();
            return;
        }
    }
    else if (FlashState == EFlashState::CompleteBlind)
    {
        // 완전 실명 상태: 강도 1.0 유지
        CurrentFlashIntensity = 1.0f;
    }
    else if (FlashState == EFlashState::Recovery)
    {
        // 회복 상태: 발로란트 스타일의 빠른 감소
        float RecoveryRatio = FMath::Clamp(ElapsedTime / m_RecoveryDuration, 0.0f, 1.0f);
        
        // 더 빠른 초기 감소를 위해 지수를 2.0으로 변경 (기존 3.0)
        float RecoveryFactor = FMath::Pow(1.0f - RecoveryRatio, 2.0f);
        CurrentFlashIntensity = RecoveryFactor;
        
        // 회복 완료 체크
        if (RecoveryRatio >= 1.0f || CurrentFlashIntensity <= 0.01f)
        {
            StopFlashEffect();
            return;
        }
    }

    // 델리게이트 호출 - 섬광 위치 정보도 함께 전달
    OnFlashIntensityChanged.Broadcast(CurrentFlashIntensity, FlashLocation);

}

void UFlashComponent::StopFlashEffect()
{
    FlashState = EFlashState::None;
    CurrentFlashIntensity = 0.0f;
    ElapsedTime = 0.0f;
    ViewAngleMultiplier = 1.0f;
    bIsMinimumFlash = false;
    CurrentFlashType = EFlashType::Default;
    FlashLocation = FVector::ZeroVector;

    // 타이머 정리
    if (BlindToRecoveryTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(BlindToRecoveryTimer);
    }

    // 틱 비활성화
    SetComponentTickEnabled(false);

    // 델리게이트 호출
    OnFlashIntensityChanged.Broadcast(CurrentFlashIntensity, FlashLocation);
    
    UE_LOG(LogTemp, Warning, TEXT("섬광 효과 종료"));
}