#include "HealingOrbActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"

AHealingOrbActor::AHealingOrbActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    
    // 루트 컴포넌트
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    
    // 오브 메시
    OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
    OrbMesh->SetupAttachment(GetRootComponent());
    OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // 기본 구체 메시 설정
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        OrbMesh->SetStaticMesh(SphereMesh.Object);
        OrbMesh->SetRelativeScale3D(FVector(0.3f));
    }
    
    // 오브 이펙트
    OrbEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OrbEffect"));
    OrbEffect->SetupAttachment(OrbMesh);
    OrbEffect->bAutoActivate = true;
    
    // 하이라이트 이펙트
    HighlightEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HighlightEffect"));
    HighlightEffect->SetupAttachment(OrbMesh);
    HighlightEffect->bAutoActivate = false;
    
    // 포인트 라이트
    OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
    OrbLight->SetupAttachment(OrbMesh);
    OrbLight->SetIntensity(500.0f);
    OrbLight->SetAttenuationRadius(200.0f);
    OrbLight->SetLightColor(NormalColor);
    OrbLight->SetCastShadows(false);
}

void AHealingOrbActor::SetIsReplicated(bool bShouldReplicate)
{
    SetReplicates(bShouldReplicate);
    SetReplicateMovement(bShouldReplicate);
    
    // 컴포넌트들도 복제 설정
    if (OrbMesh)
    {
        OrbMesh->SetIsReplicated(bShouldReplicate);
    }
    
    if (OrbEffect)
    {
        OrbEffect->SetIsReplicated(bShouldReplicate);
    }
    
    if (HighlightEffect)
    {
        HighlightEffect->SetIsReplicated(bShouldReplicate);
    }
    
    if (OrbLight)
    {
        OrbLight->SetIsReplicated(bShouldReplicate);
    }
}

void AHealingOrbActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AHealingOrbActor, bIsHighlighted);
}

void AHealingOrbActor::BeginPlay()
{
    Super::BeginPlay();
    
    BaseScale = OrbMesh->GetRelativeScale3D();
    
    // 머티리얼 인스턴스 생성 및 설정
    if (OrbMesh->GetStaticMesh())
    {
        UMaterialInstanceDynamic* DynMaterial = OrbMesh->CreateAndSetMaterialInstanceDynamic(0);
        if (DynMaterial)
        {
            DynMaterial->SetVectorParameterValue("EmissiveColor", NormalColor);
            DynMaterial->SetScalarParameterValue("EmissiveIntensity", 3.0f);
        }
    }
    
    // 아이들 사운드 재생
    if (OrbIdleSound)
    {
        IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
            OrbIdleSound, 
            OrbMesh, 
            NAME_None, 
            FVector::ZeroVector, 
            EAttachLocation::KeepRelativeOffset, 
            true
        );
    }
    
    // 초기 가시성 업데이트
    UpdateVisibility();
}

void AHealingOrbActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 오브 회전
    FRotator CurrentRotation = OrbMesh->GetRelativeRotation();
    CurrentRotation.Yaw += OrbRotationSpeed * DeltaTime;
    OrbMesh->SetRelativeRotation(CurrentRotation);
    
    // 오브 펄스 효과
    CurrentPulseTime += DeltaTime * OrbPulseSpeed;
    float PulseValue = FMath::Sin(CurrentPulseTime) * OrbPulseScale + 1.0f;
    OrbMesh->SetRelativeScale3D(BaseScale * PulseValue);
    
    // 라이트 강도 펄스
    if (OrbLight)
    {
        float LightIntensity = bIsHighlighted ? 1000.0f : 500.0f;
        LightIntensity *= PulseValue;
        OrbLight->SetIntensity(LightIntensity);
    }
}

void AHealingOrbActor::SetTargetHighlight(bool bHighlight)
{
    if (bIsHighlighted == bHighlight)
        return;
    
    bIsHighlighted = bHighlight;
    
    // 하이라이트 이펙트 활성화/비활성화
    if (HighlightEffect)
    {
        if (bHighlight)
        {
            HighlightEffect->Activate();
        }
        else
        {
            HighlightEffect->Deactivate();
        }
    }
    
    // 색상 변경
    FLinearColor TargetColor = bHighlight ? HighlightColor : NormalColor;
    
    // 라이트 색상 변경
    if (OrbLight)
    {
        OrbLight->SetLightColor(TargetColor);
    }
    
    // 머티리얼 색상 변경
    if (OrbMesh)
    {
        UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(OrbMesh->GetMaterial(0));
        if (DynMaterial)
        {
            DynMaterial->SetVectorParameterValue("EmissiveColor", TargetColor);
        }
    }
    
    // 하이라이트 사운드 재생
    if (bHighlight && OrbHighlightSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), OrbHighlightSound, GetActorLocation());
    }
}

void AHealingOrbActor::SetOnlyOwnerSee(bool bNewOnlyOwnerSee)
{
    if (OrbMesh)
    {
        OrbMesh->SetOnlyOwnerSee(bNewOnlyOwnerSee);
    }
    
    if (OrbEffect)
    {
        OrbEffect->SetOnlyOwnerSee(bNewOnlyOwnerSee);
    }
    
    if (HighlightEffect)
    {
        HighlightEffect->SetOnlyOwnerSee(bNewOnlyOwnerSee);
    }
    
    // PointLight는 SetVisibility로 제어
    if (OrbLight && bNewOnlyOwnerSee)
    {
        OrbLight->SetVisibility(true);
    }
}

void AHealingOrbActor::SetOwnerNoSee(bool bNewOwnerNoSee)
{
    if (OrbMesh)
    {
        OrbMesh->SetOwnerNoSee(bNewOwnerNoSee);
    }
    
    if (OrbEffect)
    {
        OrbEffect->SetOwnerNoSee(bNewOwnerNoSee);
    }
    
    if (HighlightEffect)
    {
        HighlightEffect->SetOwnerNoSee(bNewOwnerNoSee);
    }
    
    // PointLight는 SetVisibility로 제어
    UpdateVisibility();
}

void AHealingOrbActor::SetIsOwnerOnly(bool bIsOwnerOnly)
{
    this->IsOwnerOnly = bIsOwnerOnly;
    SetOwnerNoSee(bIsOwnerOnly);
    //UpdateVisibility();
}

void AHealingOrbActor::UpdateVisibility()
{
    if (!OrbLight)
        return;
    
    // 로컬 플레이어가 오너인지 확인
    bool bIsLocalOwner = false;
    if (GetOwner())
    {
        APawn* OwnerPawn = Cast<APawn>(GetOwner());
        if (OwnerPawn && OwnerPawn->IsLocallyControlled())
        {
            bIsLocalOwner = true;
        }
    }
    
    // bIsOwnerOnly가 true면 오너가 아닌 경우에만 라이트 표시
    if (IsOwnerOnly)
    {
        OrbLight->SetVisibility(!bIsLocalOwner);
    }
    else
    {
        OrbLight->SetVisibility(true);
    }
}

void AHealingOrbActor::UpdateHighlightState_Implementation(bool bHighlight)
{
    SetTargetHighlight(bHighlight);
}