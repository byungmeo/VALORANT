#include "BarrierOrbActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"

ABarrierOrbActor::ABarrierOrbActor()
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
        OrbMesh->SetRelativeScale3D(FVector(0.25f));
    }
    
    // 오브 이펙트
    OrbEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OrbEffect"));
    OrbEffect->SetupAttachment(OrbMesh);
    OrbEffect->bAutoActivate = true;
    OrbEffect->SetIsReplicated(true);
    
    // 포인트 라이트
    OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
    OrbLight->SetupAttachment(OrbMesh);
    OrbLight->SetIntensity(800.0f);
    OrbLight->SetAttenuationRadius(150.0f);
    OrbLight->SetLightColor(OrbColor);
    OrbLight->SetCastShadows(false);
}

void ABarrierOrbActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(ABarrierOrbActor, bIsPlacementValid);
    DOREPLIFETIME(ABarrierOrbActor, OrbViewType);
}

void ABarrierOrbActor::BeginPlay()
{
    Super::BeginPlay();
    
    InitialRelativeLocation = OrbMesh->GetRelativeLocation();
    
    // 머티리얼 인스턴스 생성 및 설정
    if (OrbMesh->GetStaticMesh())
    {
        UMaterialInstanceDynamic* DynMaterial = OrbMesh->CreateAndSetMaterialInstanceDynamic(0);
        if (DynMaterial)
        {
            DynMaterial->SetVectorParameterValue("EmissiveColor", OrbColor);
            DynMaterial->SetScalarParameterValue("EmissiveIntensity", 5.0f);
        }
    }
    
    // Owner가 이미 설정되어 있으면 가시성 업데이트
    if (GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void ABarrierOrbActor::OnRep_Owner()
{
    Super::OnRep_Owner();
    
    // Owner가 변경되면 가시성 설정 업데이트
    UpdateVisibilitySettings();
}

void ABarrierOrbActor::SetOrbViewType(EBarrierOrbViewType ViewType)
{
    OrbViewType = ViewType;
    
    // 서버에서 타입 변경
    if (HasAuthority())
    {
        OnRep_OrbViewType();
    }
}

void ABarrierOrbActor::OnRep_OrbViewType()
{
    UpdateVisibilitySettings();
}

void ABarrierOrbActor::UpdateVisibilitySettings()
{
    if (!GetOwner())
        return;
        
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
        return;
    
    // 로컬 플레이어 컨트롤러 가져오기
    APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
    bool bIsLocalOwner = LocalPC && LocalPC->GetPawn() == OwnerPawn;
    
    if (OrbViewType == EBarrierOrbViewType::FirstPerson)
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
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 숨김
            OrbMesh->SetVisibility(false, true);
            OrbEffect->SetVisibility(false, true);
            OrbLight->SetVisibility(false);
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
        }
    }
    
    // 사운드 처리 (처음 한 번만)
    if (!bVisibilityInitialized && OrbIdleSound)
    {
        bool bShouldPlaySound = (OrbViewType == EBarrierOrbViewType::FirstPerson && bIsLocalOwner) ||
                               (OrbViewType == EBarrierOrbViewType::ThirdPerson && !bIsLocalOwner);
        
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
}

void ABarrierOrbActor::Tick(float DeltaTime)
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
        // 오브 회전
        FRotator CurrentRotation = OrbMesh->GetRelativeRotation();
        CurrentRotation.Yaw += OrbRotationSpeed * DeltaTime;
        OrbMesh->SetRelativeRotation(CurrentRotation);
        
        // 오브 상하 부유
        CurrentFloatTime += DeltaTime * OrbFloatSpeed;
        float FloatOffset = FMath::Sin(CurrentFloatTime) * OrbFloatHeight;
        FVector NewLocation = InitialRelativeLocation + FVector(0, 0, FloatOffset);
        OrbMesh->SetRelativeLocation(NewLocation);
    }
}

void ABarrierOrbActor::SetPlacementValid(bool bValid)
{
    if (HasAuthority())
    {
        bIsPlacementValid = bValid;
        OnRep_IsPlacementValid();
    }
}

void ABarrierOrbActor::OnRep_IsPlacementValid()
{
    UpdatePlacementVisuals();
}

void ABarrierOrbActor::UpdatePlacementVisuals()
{
    // 보이는 경우에만 배치 상태 업데이트
    if (!OrbMesh || !OrbMesh->IsVisible())
        return;
    
    // 배치 가능 여부에 따라 색상 변경
    FLinearColor TargetColor = bIsPlacementValid ? OrbColor : FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);  // 빨간색
    
    // 라이트 색상 변경
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
}