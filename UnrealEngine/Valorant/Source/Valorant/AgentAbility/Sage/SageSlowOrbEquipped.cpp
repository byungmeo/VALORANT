#include "SageSlowOrbEquipped.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"

ASageSlowOrbEquipped::ASageSlowOrbEquipped()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    
    // 루트 컴포넌트
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    
    // 슬로우 오브 메시 설정
    SlowOrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlowOrbMesh"));
    SlowOrbMesh->SetupAttachment(GetRootComponent());
    SlowOrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SlowOrbMesh->SetIsReplicated(true);
    
    // 메시 에셋 설정 (구체 메시 사용)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (MeshAsset.Succeeded())
    {
        SlowOrbMesh->SetStaticMesh(MeshAsset.Object);
    }
    
    // 머티리얼 설정 (세이지 슬로우 오브용 머티리얼)
    static ConstructorHelpers::FObjectFinder<UMaterial> SlowOrbMaterial(TEXT("/Script/Engine.Material'/Engine/VREditor/LaserPointer/LaserPointerMaterial.LaserPointerMaterial'"));
    if (SlowOrbMaterial.Succeeded())
    {
        SlowOrbMesh->SetMaterial(0, SlowOrbMaterial.Object);
    }
    
    // 상대적 위치 및 크기 설정
    SlowOrbMesh->SetRelativeScale3D(FVector(0.3f));
    SlowOrbMesh->SetRelativeLocation(FVector(10, 0, 0));
    
    // 글로우 이펙트 컴포넌트
    GlowEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GlowEffect"));
    GlowEffectComponent->SetupAttachment(SlowOrbMesh);
    GlowEffectComponent->bAutoActivate = true;
    GlowEffectComponent->SetIsReplicated(true);
    
    // 트레일 이펙트 컴포넌트
    TrailEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
    TrailEffectComponent->SetupAttachment(SlowOrbMesh);
    TrailEffectComponent->bAutoActivate = false;
    TrailEffectComponent->SetIsReplicated(true);
}

void ASageSlowOrbEquipped::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(ASageSlowOrbEquipped, SlowOrbViewType);
    DOREPLIFETIME(ASageSlowOrbEquipped, bIsEquipped);
}

void ASageSlowOrbEquipped::BeginPlay()
{
    Super::BeginPlay();
    
    BaseScale = SlowOrbMesh->GetRelativeScale3D();
    BaseLocation = SlowOrbMesh->GetRelativeLocation();
    
    // 슬로우 오브 이펙트 설정
    if (SlowOrbGlowEffect && GlowEffectComponent)
    {
        GlowEffectComponent->SetAsset(SlowOrbGlowEffect);
    }
    
    if (SlowOrbTrailEffect && TrailEffectComponent)
    {
        TrailEffectComponent->SetAsset(SlowOrbTrailEffect);
    }
    
    // Owner가 이미 설정되어 있으면 가시성 업데이트
    if (GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void ASageSlowOrbEquipped::OnRep_Owner()
{
    Super::OnRep_Owner();
    
    // Owner가 변경되면 가시성 설정 업데이트
    UpdateVisibilitySettings();
}

void ASageSlowOrbEquipped::SetSlowOrbViewType(EViewType ViewType)
{
    SlowOrbViewType = ViewType;
    
    // 서버에서 타입 변경
    if (HasAuthority())
    {
        OnRep_SlowOrbViewType();
    }
}

void ASageSlowOrbEquipped::OnRep_SlowOrbViewType()
{
    UpdateVisibilitySettings();
}

void ASageSlowOrbEquipped::UpdateVisibilitySettings()
{
    if (!GetOwner())
        return;
        
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
        return;
    
    // 로컬 플레이어 컨트롤러 가져오기
    APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
    bool bIsLocalOwner = LocalPC && LocalPC->GetPawn() == OwnerPawn;
    
    if (SlowOrbViewType == EViewType::FirstPerson)
    {
        // 1인칭 슬로우 오브 - 오직 오너만 볼 수 있음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 보이게 설정
            SlowOrbMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            TrailEffectComponent->SetVisibility(true, true);
            
            // Owner만 보기 설정
            SlowOrbMesh->SetOnlyOwnerSee(true);
            GlowEffectComponent->SetOnlyOwnerSee(true);
            TrailEffectComponent->SetOnlyOwnerSee(true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 숨김
            SlowOrbMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
            TrailEffectComponent->SetVisibility(false, true);
        }
    }
    else  // ThirdPerson
    {
        // 3인칭 슬로우 오브 - 오너는 볼 수 없음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 숨김
            SlowOrbMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
            TrailEffectComponent->SetVisibility(false, true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 보이게 설정
            SlowOrbMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            TrailEffectComponent->SetVisibility(true, true);
            
            // Owner는 못보게 설정
            SlowOrbMesh->SetOwnerNoSee(true);
            GlowEffectComponent->SetOwnerNoSee(true);
            TrailEffectComponent->SetOwnerNoSee(true);
        }
    }
    
    // 사운드 처리 (처음 한 번만)
    if (!bVisibilityInitialized && SlowOrbIdleSound && bIsEquipped)
    {
        bool bShouldPlaySound = (SlowOrbViewType == EViewType::FirstPerson && bIsLocalOwner) ||
                               (SlowOrbViewType == EViewType::ThirdPerson && !bIsLocalOwner);
        
        if (bShouldPlaySound)
        {
            IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
                SlowOrbIdleSound, 
                SlowOrbMesh, 
                NAME_None, 
                FVector::ZeroVector, 
                EAttachLocation::KeepRelativeOffset, 
                true
            );
        }
        
        bVisibilityInitialized = true;
    }
}

void ASageSlowOrbEquipped::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 가시성이 설정되지 않았으면 재시도
    if (!bVisibilityInitialized && GetOwner())
    {
        UpdateVisibilitySettings();
    }
    
    // 슬로우 오브가 보이고 장착된 경우에만 애니메이션 적용
    if (SlowOrbMesh && SlowOrbMesh->IsVisible() && bIsEquipped)
    {
        // 오브 떠다니는 효과 (위아래로 부드럽게 움직임)
        CurrentFloatTime += DeltaTime * OrbFloatSpeed;
        float FloatOffset = FMath::Sin(CurrentFloatTime) * OrbFloatHeight;
        FVector NewLocation = BaseLocation + FVector(0, 0, FloatOffset);
        SlowOrbMesh->SetRelativeLocation(NewLocation);
        
        // 오브 회전
        CurrentRotation += OrbRotationSpeed * DeltaTime;
        SlowOrbMesh->SetRelativeRotation(FRotator(0, CurrentRotation, 0));
        
        // 오브 펄스 효과
        CurrentPulseTime += DeltaTime * OrbPulseSpeed;
        float PulseValue = FMath::Sin(CurrentPulseTime) * OrbPulseScale + 1.0f;
        SlowOrbMesh->SetRelativeScale3D(BaseScale * PulseValue);
    }
}

void ASageSlowOrbEquipped::OnEquip()
{
    if (HasAuthority())
    {
        bIsEquipped = true;
        OnRep_IsEquipped();
        MulticastPlayEquipSound();
    }
}

void ASageSlowOrbEquipped::OnUnequip()
{
    if (HasAuthority())
    {
        bIsEquipped = false;
        OnRep_IsEquipped();
        MulticastPlayUnequipSound();
    }
}

void ASageSlowOrbEquipped::OnRep_IsEquipped()
{
    UpdateEquipVisuals();
}

void ASageSlowOrbEquipped::UpdateEquipVisuals()
{
    // 보이는 경우에만 장착 시각 효과 업데이트
    if (!SlowOrbMesh || !SlowOrbMesh->IsVisible())
        return;
    
    // 이펙트 활성화/비활성화
    if (bIsEquipped)
    {
        if (GlowEffectComponent)
        {
            GlowEffectComponent->Activate();
        }
        if (TrailEffectComponent)
        {
            TrailEffectComponent->Activate();
        }
        MulticastPlayIdleSound();
    }
    else
    {
        if (GlowEffectComponent)
        {
            GlowEffectComponent->Deactivate();
        }
        if (TrailEffectComponent)
        {
            TrailEffectComponent->Deactivate();
        }
    }
}

void ASageSlowOrbEquipped::MulticastPlayEquipSound_Implementation()
{
    if (SlowOrbEquipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SlowOrbEquipSound, GetActorLocation());
    }
}

void ASageSlowOrbEquipped::MulticastPlayUnequipSound_Implementation()
{
    if (SlowOrbUnequipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SlowOrbUnequipSound, GetActorLocation());
    }
}

void ASageSlowOrbEquipped::MulticastPlayIdleSound_Implementation()
{
    if (SlowOrbIdleSound && !IdleAudioComponent)
    {
        IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
            SlowOrbIdleSound, 
            SlowOrbMesh, 
            NAME_None, 
            FVector::ZeroVector, 
            EAttachLocation::KeepRelativeOffset, 
            true
        );
    }
}