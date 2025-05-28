#include "HealingOrbActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

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
    
    // 아이들 사운드 재생 (로컬에서만)
    if (OrbIdleSound && GetOwner())
    {
        APawn* OwnerPawn = Cast<APawn>(GetOwner());
        if ((OrbViewType == EOrbViewType::FirstPerson && OwnerPawn && OwnerPawn->IsLocallyControlled()) ||
            (OrbViewType == EOrbViewType::ThirdPerson))
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
    }
    
    // 가시성 설정은 SetOrbViewType이 호출된 후에 처리됨
}

void AHealingOrbActor::SetOrbViewType(EOrbViewType ViewType)
{
    OrbViewType = ViewType;
    UpdateVisibilitySettings();
}

void AHealingOrbActor::UpdateVisibilitySettings()
{
    if (!GetOwner())
        return;
        
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
        return;
    
    if (OrbViewType == EOrbViewType::FirstPerson)
    {
        // 1인칭 오브 - 오직 오너만 볼 수 있음
        OrbMesh->SetOnlyOwnerSee(true);
        OrbEffect->SetOnlyOwnerSee(true);
        HighlightEffect->SetOnlyOwnerSee(true);
        OrbLight->SetVisibility(true);
        
        // 복제 비활성화
        SetReplicates(false);
    }
    else  // ThirdPerson
    {
        // 3인칭 오브 - 오너는 볼 수 없음
        OrbMesh->SetOwnerNoSee(true);
        OrbEffect->SetOwnerNoSee(true);
        HighlightEffect->SetOwnerNoSee(true);
        
        // 라이트는 로컬 플레이어가 오너가 아닐 때만 표시
        bool bIsLocalOwner = OwnerPawn->IsLocallyControlled();
        OrbLight->SetVisibility(!bIsLocalOwner);
        
        // 복제 활성화
        SetReplicates(true);
    }
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
    
    // 라이트 강도 펄스 (가시성 체크)
    if (OrbLight && OrbLight->IsVisible())
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
    
    // 로컬에서 즉시 업데이트
    UpdateHighlightVisuals();
}

void AHealingOrbActor::OnRep_IsHighlighted()
{
    // 복제된 값이 변경되었을 때 비주얼 업데이트
    UpdateHighlightVisuals();
}

void AHealingOrbActor::UpdateHighlightVisuals()
{
    // 하이라이트 이펙트 활성화/비활성화
    if (HighlightEffect)
    {
        if (bIsHighlighted)
        {
            HighlightEffect->Activate();
        }
        else
        {
            HighlightEffect->Deactivate();
        }
    }
    
    // 색상 변경
    FLinearColor TargetColor = bIsHighlighted ? HighlightColor : NormalColor;
    
    // 라이트 색상 변경 (가시성 체크)
    if (OrbLight && OrbLight->IsVisible())
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
    
    // 하이라이트 사운드 재생 (로컬에서만, 적절한 경우에만)
    if (bIsHighlighted && OrbHighlightSound && IsValid(this))
    {
        // 1인칭이면 오너가 로컬일 때만, 3인칭이면 항상 재생
        APawn* OwnerPawn = Cast<APawn>(GetOwner());
        bool bShouldPlaySound = false;
        
        if (OrbViewType == EOrbViewType::FirstPerson)
        {
            bShouldPlaySound = OwnerPawn && OwnerPawn->IsLocallyControlled();
        }
        else
        {
            bShouldPlaySound = true;
        }
        
        if (bShouldPlaySound)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), OrbHighlightSound, GetActorLocation());
        }
    }
}