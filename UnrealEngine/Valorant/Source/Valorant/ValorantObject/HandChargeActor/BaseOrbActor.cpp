#include "BaseOrbActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"

ABaseOrbActor::ABaseOrbActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    
    // 루트 컴포넌트
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    
    // 오브 메시
    OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
    OrbMesh->SetupAttachment(GetRootComponent());
    OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OrbMesh->SetIsReplicated(true);
    
    // 기본 구체 메시 설정
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        OrbMesh->SetStaticMesh(SphereMesh.Object);
        OrbMesh->SetRelativeScale3D(FVector(OrbScale));
    }
    
    // 오브 이펙트
    OrbEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OrbEffect"));
    OrbEffect->SetupAttachment(OrbMesh);
    OrbEffect->bAutoActivate = true;
    OrbEffect->SetIsReplicated(true);
    
    // 포인트 라이트
    OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
    OrbLight->SetupAttachment(OrbMesh);
    OrbLight->SetIntensity(LightIntensity);
    OrbLight->SetAttenuationRadius(LightRadius);
    OrbLight->SetLightColor(OrbColor);
    OrbLight->SetCastShadows(false);
}

void ABaseOrbActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(ABaseOrbActor, OrbViewType);
}

void ABaseOrbActor::BeginPlay()
{
    Super::BeginPlay();
    
    InitialRelativeLocation = OrbMesh->GetRelativeLocation();
    BaseScale = OrbMesh->GetRelativeScale3D();
    
    // 머티리얼 인스턴스 생성 및 설정
    if (OrbMesh->GetStaticMesh())
    {
        UMaterialInstanceDynamic* DynMaterial = OrbMesh->CreateAndSetMaterialInstanceDynamic(0);
        if (DynMaterial)
        {
            DynMaterial->SetVectorParameterValue("EmissiveColor", OrbColor);
            DynMaterial->SetScalarParameterValue("EmissiveIntensity", EmissiveIntensity);
        }
    }
    
    // Owner가 이미 설정되어 있으면 가시성 업데이트
    if (GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void ABaseOrbActor::OnRep_Owner()
{
    Super::OnRep_Owner();
    
    // Owner가 변경되면 가시성 설정 업데이트
    UpdateVisibilitySettings();
}

EOrbViewType ABaseOrbActor::GetOrbViewType()
{
    return OrbViewType;
}

void ABaseOrbActor::SetOrbViewType(EOrbViewType ViewType)
{
    OrbViewType = ViewType;
    
    // 서버에서 타입 변경
    if (HasAuthority())
    {
        OnRep_OrbViewType();
    }
}

void ABaseOrbActor::OnRep_OrbViewType()
{
    UpdateVisibilitySettings();
}

FVector ABaseOrbActor::GetBaseScale() const
{
    return BaseScale;
}

void ABaseOrbActor::UpdateVisibilitySettings()
{
    if (!GetOwner())
        return;
        
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
        return;
    
    // 로컬 플레이어 컨트롤러 가져오기
    APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
    bool bIsLocalOwner = LocalPC && LocalPC->GetPawn() == OwnerPawn;
    
    bool bShouldBeVisible = false;
    
    if (OrbViewType == EOrbViewType::FirstPerson)
    {
        // 1인칭 오브 - 오직 오너만 볼 수 있음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 보이게 설정
            OrbMesh->SetVisibility(true, true);
            OrbEffect->SetVisibility(true, true);
            OrbLight->SetVisibility(true);
            
            // Owner만 보기 설정
            OrbMesh->SetOnlyOwnerSee(true);
            OrbEffect->SetOnlyOwnerSee(true);
            
            bShouldBeVisible = true;
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 숨김
            OrbMesh->SetVisibility(false, true);
            OrbEffect->SetVisibility(false, true);
            OrbLight->SetVisibility(false);
            
            bShouldBeVisible = false;
        }
    }
    else  // ThirdPerson
    {
        // 3인칭 오브 - 오너는 볼 수 없음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 숨김
            OrbMesh->SetVisibility(false, true);
            OrbEffect->SetVisibility(false, true);
            OrbLight->SetVisibility(false);
            
            bShouldBeVisible = false;
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 보이게 설정
            OrbMesh->SetVisibility(true, true);
            OrbEffect->SetVisibility(true, true);
            OrbLight->SetVisibility(true);
            
            // Owner는 못보게 설정
            OrbMesh->SetOwnerNoSee(true);
            OrbEffect->SetOwnerNoSee(true);
            
            bShouldBeVisible = true;
        }
    }
    
    // 사운드 처리 (처음 한 번만)
    if (!bVisibilityInitialized && OrbIdleSound)
    {
        bool bShouldPlaySound = (OrbViewType == EOrbViewType::FirstPerson && bIsLocalOwner) ||
                               (OrbViewType == EOrbViewType::ThirdPerson && !bIsLocalOwner);
        
        if (bShouldPlaySound)
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
        
        bVisibilityInitialized = true;
    }
    
    // 하위 클래스에 가시성 변경 알림
    OnVisibilityChanged(bShouldBeVisible);
}

void ABaseOrbActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 가시성이 설정되지 않았으면 재시도
    if (!bVisibilityInitialized && GetOwner())
    {
        UpdateVisibilitySettings();
    }
    
    // 오브가 보이는 경우에만 애니메이션 적용
    if (OrbMesh && OrbMesh->IsVisible())
    {
        // 기본 회전
        FRotator CurrentRotation = OrbMesh->GetRelativeRotation();
        CurrentRotation.Yaw += OrbRotationSpeed * DeltaTime;
        OrbMesh->SetRelativeRotation(CurrentRotation);
        
        // 애니메이션 업데이트
        UpdateAnimation(DeltaTime);
    }
}

void ABaseOrbActor::UpdateAnimation(float DeltaTime)
{
    CurrentAnimationTime += DeltaTime;
    
    switch (AnimationType)
    {
        case EOrbAnimationType::Float:
            UpdateFloatAnimation(DeltaTime);
            break;
            
        case EOrbAnimationType::Pulse:
            UpdatePulseAnimation(DeltaTime);
            break;
            
        case EOrbAnimationType::Custom:
            UpdateCustomAnimation(DeltaTime);
            break;
            
        case EOrbAnimationType::None:
        default:
            // 애니메이션 없음
            break;
    }
}

void ABaseOrbActor::UpdateFloatAnimation(float DeltaTime)
{
    // 오브 상하 부유
    float FloatOffset = FMath::Sin(CurrentAnimationTime * FloatSpeed) * FloatHeight;
    FVector NewLocation = InitialRelativeLocation + FVector(0, 0, FloatOffset);
    OrbMesh->SetRelativeLocation(NewLocation);
}

void ABaseOrbActor::UpdatePulseAnimation(float DeltaTime)
{
    // 오브 펄스 효과
    float PulseValue = FMath::Sin(CurrentAnimationTime * PulseSpeed) * PulseScale + 1.0f;
    OrbMesh->SetRelativeScale3D(BaseScale * PulseValue);
    
    // 라이트 강도 펄스
    if (OrbLight && OrbLight->IsVisible())
    {
        float CurrentLightIntensity = LightIntensity * PulseValue;
        OrbLight->SetIntensity(CurrentLightIntensity);
    }
}

void ABaseOrbActor::UpdateOrbColor(const FLinearColor& NewColor)
{
    // 라이트 색상 변경
    if (OrbLight && OrbLight->IsVisible())
    {
        OrbLight->SetLightColor(NewColor);
    }
    
    // 머티리얼 색상 변경
    if (OrbMesh)
    {
        UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(OrbMesh->GetMaterial(0));
        if (DynMaterial)
        {
            DynMaterial->SetVectorParameterValue("EmissiveColor", NewColor);
        }
    }
}