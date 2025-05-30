#include "PhoenixFireOrbActor.h"
#include "NiagaraComponent.h"
#include "Components/PointLightComponent.h"

APhoenixFireOrbActor::APhoenixFireOrbActor()
{
    // 베이스 설정 오버라이드 - 화염 테마
    OrbColor = StraightFireColor;
    OrbScale = 0.35f;  // 약간 큰 사이즈
    OrbRotationSpeed = 120.0f;  // 빠른 회전
    LightIntensity = 1200.0f;   // 밝은 빛
    LightRadius = 180.0f;
    EmissiveIntensity = 8.0f;   // 강한 발광
    
    // 커스텀 애니메이션 사용 (화염 일렁임 효과)
    AnimationType = EOrbAnimationType::Custom;
    
    // 화염 이펙트 추가
    FlameEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlameEffect"));
    FlameEffect->SetupAttachment(OrbMesh);
    FlameEffect->bAutoActivate = true;
    FlameEffect->SetIsReplicated(true);
}

void APhoenixFireOrbActor::BeginPlay()
{
    Super::BeginPlay();
    
    // 초기 화염 효과 설정
    UpdateFlameEffects();
}

void APhoenixFireOrbActor::SetThrowType(bool bIsCurved)
{
    bIsCurvedThrow = bIsCurved;
    UpdateFlameEffects();
}

void APhoenixFireOrbActor::UpdateCustomAnimation(float DeltaTime)
{
    FlameIntensityTime += DeltaTime * 4.0f;  // 빠른 일렁임
    
    // 화염 일렁임 효과 - 불규칙한 패턴
    float FlameFlicker1 = FMath::Sin(FlameIntensityTime) * 0.15f;
    float FlameFlicker2 = FMath::Sin(FlameIntensityTime * 1.7f) * 0.1f;
    float FlameFlicker3 = FMath::Sin(FlameIntensityTime * 2.3f) * 0.05f;
    
    float TotalFlicker = 1.0f + FlameFlicker1 + FlameFlicker2 + FlameFlicker3;
    
    // 스케일 일렁임
    FVector FlickerScale = GetBaseScale() * TotalFlicker;
    OrbMesh->SetRelativeScale3D(FlickerScale);
    
    // 라이트 강도 일렁임
    if (OrbLight && OrbLight->IsVisible())
    {
        float FlickerIntensity = LightIntensity * TotalFlicker;
        OrbLight->SetIntensity(FlickerIntensity);
    }
    
    // 색상 일렁임 (더 뜨거워 보이게)
    FLinearColor BaseFireColor = bIsCurvedThrow ? CurvedFireColor : StraightFireColor;
    FLinearColor FlickerColor = BaseFireColor;
    
    // 가끔씩 더 밝은 화염색으로 번쩍임
    float IntenseFlicker = FMath::Sin(FlameIntensityTime * 3.0f);
    if (IntenseFlicker > 0.7f)
    {
        FlickerColor = FLinearColor(1.0f, 1.0f, 0.8f, 1.0f);  // 밝은 노란빛
    }
    
    UpdateOrbColor(FlickerColor);
}

void APhoenixFireOrbActor::UpdateFlameEffects()
{
    // 보이는 경우에만 화염 효과 업데이트
    if (!OrbMesh || !OrbMesh->IsVisible())
        return;
    
    // 던지기 타입에 따른 색상 변경
    FLinearColor TargetColor = bIsCurvedThrow ? CurvedFireColor : StraightFireColor;
    UpdateOrbColor(TargetColor);
    
    // 화염 이펙트 파라미터 조정 (나이아가라 시스템이 있다면)
    if (FlameEffect)
    {
        // 직선 던지기: 더 집중된 화염
        // 포물선 던지기: 더 확산된 화염
        float FlameScale = bIsCurvedThrow ? 1.2f : 0.8f;
        FlameEffect->SetRelativeScale3D(FVector(FlameScale));
    }
}