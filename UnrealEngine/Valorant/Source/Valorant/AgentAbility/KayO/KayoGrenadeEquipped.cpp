#include "KayoGrenadeEquipped.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"

AKayoGrenadeEquipped::AKayoGrenadeEquipped()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    
    // 루트 컴포넌트
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    
    // 수류탄 메시 설정
    GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
    GrenadeMesh->SetupAttachment(GetRootComponent());
    GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GrenadeMesh->SetIsReplicated(true);
    
    // 메시 에셋 설정
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Script/Engine.StaticMesh'/Game/Resource/Props/Projectiles/KAYO_Ability_Grenade/KayoGrenade.KayoGrenade'"));
    if (MeshAsset.Succeeded())
    {
        GrenadeMesh->SetStaticMesh(MeshAsset.Object);
    }
    
    // 머티리얼 설정
    static ConstructorHelpers::FObjectFinder<UMaterial> GrenadeMaterial(TEXT("/Script/Engine.Material'/Game/Resource/Props/Projectiles/KAYO_Ability_Grenade/M_KayoGrenade.M_KayoGrenade'"));
    if (GrenadeMaterial.Succeeded())
    {
        GrenadeMesh->SetMaterial(0, GrenadeMaterial.Object);
    }
    
    // 상대적 위치 및 회전 설정
    GrenadeMesh->SetRelativeScale3D(FVector(0.15f));
    GrenadeMesh->SetRelativeLocation(FVector(10, 0, 0));
    GrenadeMesh->SetRelativeRotation(FRotator(0, 0, 0));
    
    // 글로우 이펙트 컴포넌트
    GlowEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GlowEffect"));
    GlowEffectComponent->SetupAttachment(GrenadeMesh);
    GlowEffectComponent->bAutoActivate = false;
    GlowEffectComponent->SetIsReplicated(true);
}

void AKayoGrenadeEquipped::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AKayoGrenadeEquipped, GrenadeViewType);
    DOREPLIFETIME(AKayoGrenadeEquipped, bIsEquipped);
    DOREPLIFETIME(AKayoGrenadeEquipped, bIsOverhand);
}

void AKayoGrenadeEquipped::BeginPlay()
{
    Super::BeginPlay();
    
    BaseScale = GrenadeMesh->GetRelativeScale3D();
    
    // 수류탄 글로우 이펙트 설정
    if (GrenadeGlowEffect && GlowEffectComponent)
    {
        GlowEffectComponent->SetAsset(GrenadeGlowEffect);
    }
    
    // Owner가 이미 설정되어 있으면 가시성 업데이트
    if (GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void AKayoGrenadeEquipped::OnRep_Owner()
{
    Super::OnRep_Owner();
    
    // Owner가 변경되면 가시성 설정 업데이트
    UpdateVisibilitySettings();
}

void AKayoGrenadeEquipped::SetGrenadeViewType(EGrenadeViewType ViewType)
{
    GrenadeViewType = ViewType;
    
    // 서버에서 타입 변경
    if (HasAuthority())
    {
        OnRep_GrenadeViewType();
    }
}

void AKayoGrenadeEquipped::OnRep_GrenadeViewType()
{
    UpdateVisibilitySettings();
}

void AKayoGrenadeEquipped::UpdateVisibilitySettings()
{
    if (!GetOwner())
        return;
        
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
        return;
    
    // 로컬 플레이어 컨트롤러 가져오기
    APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
    bool bIsLocalOwner = LocalPC && LocalPC->GetPawn() == OwnerPawn;
    
    if (GrenadeViewType == EGrenadeViewType::FirstPerson)
    {
        // 1인칭 수류탄 - 오직 오너만 볼 수 있음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 보이게 설정
            GrenadeMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            
            // Owner만 보기 설정
            GrenadeMesh->SetOnlyOwnerSee(true);
            GlowEffectComponent->SetOnlyOwnerSee(true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 숨김
            GrenadeMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
        }
    }
    else  // ThirdPerson
    {
        // 3인칭 수류탄 - 오너는 볼 수 없음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 숨김
            GrenadeMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 보이게 설정
            GrenadeMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            
            // Owner는 못보게 설정
            GrenadeMesh->SetOwnerNoSee(true);
            GlowEffectComponent->SetOwnerNoSee(true);
        }
    }
    
    // 사운드 처리 (처음 한 번만)
    if (!bVisibilityInitialized && GrenadeIdleSound && bIsEquipped)
    {
        bool bShouldPlaySound = (GrenadeViewType == EGrenadeViewType::FirstPerson && bIsLocalOwner) ||
                               (GrenadeViewType == EGrenadeViewType::ThirdPerson && !bIsLocalOwner);
        
        if (bShouldPlaySound)
        {
            IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
                GrenadeIdleSound, 
                GrenadeMesh, 
                NAME_None, 
                FVector::ZeroVector, 
                EAttachLocation::KeepRelativeOffset, 
                true
            );
        }
        
        bVisibilityInitialized = true;
    }
}

void AKayoGrenadeEquipped::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 가시성이 설정되지 않았으면 재시도
    if (!bVisibilityInitialized && GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void AKayoGrenadeEquipped::OnEquip()
{
    if (HasAuthority())
    {
        bIsEquipped = true;
        OnRep_IsEquipped();
        MulticastPlayEquipSound();
    }
}

void AKayoGrenadeEquipped::OnUnequip()
{
    if (HasAuthority())
    {
        bIsEquipped = false;
        OnRep_IsEquipped();
        MulticastPlayUnequipSound();
    }
}

void AKayoGrenadeEquipped::SetThrowType(bool bIsOverhandThrow)
{
    if (HasAuthority())
    {
        bIsOverhand = bIsOverhandThrow;
        OnRep_IsOverhand();
    }
}

void AKayoGrenadeEquipped::OnRep_IsEquipped()
{
    UpdateEquipVisuals();
}

void AKayoGrenadeEquipped::OnRep_IsOverhand()
{
    // 던지기 타입에 따른 시각적 효과 변경 (필요시 구현)
    // 예: 다른 각도로 수류탄 회전
    if (bIsOverhand)
    {
        GrenadeMesh->SetRelativeRotation(FRotator(-45, 0, 0));
    }
    else
    {
        GrenadeMesh->SetRelativeRotation(FRotator(0, 0, 0));
    }
}

void AKayoGrenadeEquipped::UpdateEquipVisuals()
{
    // 보이는 경우에만 장착 시각 효과 업데이트
    if (!GrenadeMesh || !GrenadeMesh->IsVisible())
        return;
    
    // 글로우 이펙트 활성화/비활성화
    if (GlowEffectComponent)
    {
        if (bIsEquipped)
        {
            GlowEffectComponent->Activate();
            MulticastPlayIdleSound();
        }
        else
        {
            GlowEffectComponent->Deactivate();
        }
    }
}

void AKayoGrenadeEquipped::MulticastPlayEquipSound_Implementation()
{
    if (GrenadeEquipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), GrenadeEquipSound, GetActorLocation());
    }
}

void AKayoGrenadeEquipped::MulticastPlayUnequipSound_Implementation()
{
    if (GrenadeUnequipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), GrenadeUnequipSound, GetActorLocation());
    }
}

void AKayoGrenadeEquipped::MulticastPlayIdleSound_Implementation()
{
    if (GrenadeIdleSound && !IdleAudioComponent)
    {
        IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
            GrenadeIdleSound, 
            GrenadeMesh, 
            NAME_None, 
            FVector::ZeroVector, 
            EAttachLocation::KeepRelativeOffset, 
            true
        );
    }
}