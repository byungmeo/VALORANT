#include "KayoFlashbangEquipped.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"

AKayoFlashbangEquipped::AKayoFlashbangEquipped()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    
    // 루트 컴포넌트
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    
    // 플래시뱅 메시 설정
    FlashbangMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FlashbangMesh"));
    FlashbangMesh->SetupAttachment(GetRootComponent());
    FlashbangMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FlashbangMesh->SetIsReplicated(true);
    
    // 메시 에셋 설정
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Props/Projectiles/KAYO_Ability_Flashbang/AB_Grenadier_S0_4_Skelmesh.AB_Grenadier_S0_4_Skelmesh'"));
    if (MeshAsset.Succeeded())
    {
        FlashbangMesh->SetSkeletalMesh(MeshAsset.Object);
    }
    
    // 상대적 위치 및 회전 설정
    FlashbangMesh->SetRelativeScale3D(FVector(1.0f));
    FlashbangMesh->SetRelativeLocation(FVector(5, 0, 0));
    FlashbangMesh->SetRelativeRotation(FRotator(0, 90, 0));
    
    // 글로우 이펙트 컴포넌트
    GlowEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GlowEffect"));
    GlowEffectComponent->SetupAttachment(FlashbangMesh);
    GlowEffectComponent->bAutoActivate = false;
    GlowEffectComponent->SetIsReplicated(true);
}

void AKayoFlashbangEquipped::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AKayoFlashbangEquipped, FlashbangViewType);
    DOREPLIFETIME(AKayoFlashbangEquipped, bIsEquipped);
    DOREPLIFETIME(AKayoFlashbangEquipped, bIsAltFire);
}

void AKayoFlashbangEquipped::BeginPlay()
{
    Super::BeginPlay();
    
    BaseScale = FlashbangMesh->GetRelativeScale3D();
    
    // 플래시뱅 글로우 이펙트 설정
    if (FlashbangGlowEffect && GlowEffectComponent)
    {
        GlowEffectComponent->SetAsset(FlashbangGlowEffect);
    }
    
    // Owner가 이미 설정되어 있으면 가시성 업데이트
    if (GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void AKayoFlashbangEquipped::OnRep_Owner()
{
    Super::OnRep_Owner();
    
    // Owner가 변경되면 가시성 설정 업데이트
    UpdateVisibilitySettings();
}

void AKayoFlashbangEquipped::SetFlashbangViewType(EFlashbangViewType ViewType)
{
    FlashbangViewType = ViewType;
    
    // 서버에서 타입 변경
    if (HasAuthority())
    {
        OnRep_FlashbangViewType();
    }
}

void AKayoFlashbangEquipped::OnRep_FlashbangViewType()
{
    UpdateVisibilitySettings();
}

void AKayoFlashbangEquipped::UpdateVisibilitySettings()
{
    if (!GetOwner())
        return;
        
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
        return;
    
    // 로컬 플레이어 컨트롤러 가져오기
    APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
    bool bIsLocalOwner = LocalPC && LocalPC->GetPawn() == OwnerPawn;
    
    if (FlashbangViewType == EFlashbangViewType::FirstPerson)
    {
        // 1인칭 플래시뱅 - 오직 오너만 볼 수 있음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 보이게 설정
            FlashbangMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            
            // Owner만 보기 설정
            FlashbangMesh->SetOnlyOwnerSee(true);
            GlowEffectComponent->SetOnlyOwnerSee(true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 숨김
            FlashbangMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
        }
    }
    else  // ThirdPerson
    {
        // 3인칭 플래시뱅 - 오너는 볼 수 없음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 숨김
            FlashbangMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 보이게 설정
            FlashbangMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            
            // Owner는 못보게 설정
            FlashbangMesh->SetOwnerNoSee(true);
            GlowEffectComponent->SetOwnerNoSee(true);
        }
    }
    
    // 사운드 처리 (처음 한 번만)
    if (!bVisibilityInitialized && FlashbangIdleSound && bIsEquipped)
    {
        bool bShouldPlaySound = (FlashbangViewType == EFlashbangViewType::FirstPerson && bIsLocalOwner) ||
                               (FlashbangViewType == EFlashbangViewType::ThirdPerson && !bIsLocalOwner);
        
        if (bShouldPlaySound)
        {
            IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
                FlashbangIdleSound, 
                FlashbangMesh, 
                NAME_None, 
                FVector::ZeroVector, 
                EAttachLocation::KeepRelativeOffset, 
                true
            );
        }
        
        bVisibilityInitialized = true;
    }
}

void AKayoFlashbangEquipped::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 가시성이 설정되지 않았으면 재시도
    if (!bVisibilityInitialized && GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void AKayoFlashbangEquipped::OnEquip()
{
    if (HasAuthority())
    {
        bIsEquipped = true;
        OnRep_IsEquipped();
        MulticastPlayEquipSound();
    }
}

void AKayoFlashbangEquipped::OnUnequip()
{
    if (HasAuthority())
    {
        bIsEquipped = false;
        OnRep_IsEquipped();
        MulticastPlayUnequipSound();
    }
}

void AKayoFlashbangEquipped::SetThrowPreview(bool bIsAltFirePreview)
{
    if (HasAuthority())
    {
        bIsAltFire = bIsAltFirePreview;
        OnRep_IsAltFire();
    }
}

void AKayoFlashbangEquipped::OnRep_IsEquipped()
{
    UpdateEquipVisuals();
}

void AKayoFlashbangEquipped::OnRep_IsAltFire()
{
    // 던지기 타입에 따른 시각적 효과 변경
    if (bIsAltFire)
    {
        // 포물선 던지기 미리보기 - 손목을 위로 향하게
        FlashbangMesh->SetRelativeRotation(FRotator(-30, 90, 0));
        FlashbangMesh->SetRelativeLocation(FVector(5, 0, 5));  // 약간 위로
        
        // 궤적 미리보기 이펙트 표시 (선택사항)
        if (TrajectoryPreviewEffect && GlowEffectComponent)
        {
            // 궤적 프리뷰 효과
        }
    }
    else
    {
        // 직선 던지기 미리보기 - 정면을 향하게
        FlashbangMesh->SetRelativeRotation(FRotator(0, 90, 0));
        FlashbangMesh->SetRelativeLocation(FVector(5, 0, 0));  // 기본 위치
    }
}

void AKayoFlashbangEquipped::UpdateEquipVisuals()
{
    // 보이는 경우에만 장착 시각 효과 업데이트
    if (!FlashbangMesh || !FlashbangMesh->IsVisible())
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

void AKayoFlashbangEquipped::MulticastPlayEquipSound_Implementation()
{
    if (FlashbangEquipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), FlashbangEquipSound, GetActorLocation());
    }
}

void AKayoFlashbangEquipped::MulticastPlayUnequipSound_Implementation()
{
    if (FlashbangUnequipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), FlashbangUnequipSound, GetActorLocation());
    }
}

void AKayoFlashbangEquipped::MulticastPlayIdleSound_Implementation()
{
    if (FlashbangIdleSound && !IdleAudioComponent)
    {
        IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
            FlashbangIdleSound, 
            FlashbangMesh, 
            NAME_None, 
            FVector::ZeroVector, 
            EAttachLocation::KeepRelativeOffset, 
            true
        );
    }
}