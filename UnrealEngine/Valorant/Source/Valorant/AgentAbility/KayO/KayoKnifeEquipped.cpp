#include "KayoKnifeEquipped.h"
#include "KayoKnifeAnim.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"

AKayoKnifeEquipped::AKayoKnifeEquipped()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    
    // 루트 컴포넌트
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    
    // 나이프 메시 설정
    KnifeMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("KnifeMesh"));
    KnifeMesh->SetupAttachment(GetRootComponent());
    KnifeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    KnifeMesh->SetIsReplicated(true);

    // 메시 에셋 설정
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Agent/KayO/Ability/KayO_AbilityE_ZeroPoint/AB_Grenadier_S0_E_Knife_Skelmesh.AB_Grenadier_S0_E_Knife_Skelmesh'"));
    if (MeshAsset.Succeeded())
    {
        KnifeMesh->SetSkeletalMesh(MeshAsset.Object);
    }

    // 애니메이션 블루프린트 설정
    static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClass(TEXT("/Script/Engine.AnimBlueprint'/Game/Resource/Agent/KayO/Ability/KayO_AbilityE_ZeroPoint/ABP_Knife.ABP_Knife_C'"));
    if (AnimInstanceClass.Succeeded())
    {
        KnifeMesh->SetAnimInstanceClass(AnimInstanceClass.Class);
    }

    // 상대적 위치 및 회전 설정
    KnifeMesh->SetRelativeScale3D(FVector(1.0f));
    KnifeMesh->SetRelativeLocation(FVector(0, 0, 0));
    KnifeMesh->SetRelativeRotation(FRotator(0, 0, 0));
    
    // 글로우 이펙트 컴포넌트
    GlowEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GlowEffect"));
    GlowEffectComponent->SetupAttachment(KnifeMesh);
    GlowEffectComponent->bAutoActivate = false;
    GlowEffectComponent->SetIsReplicated(true);
}

void AKayoKnifeEquipped::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AKayoKnifeEquipped, KnifeViewType);
    DOREPLIFETIME(AKayoKnifeEquipped, bIsEquipped);
}

void AKayoKnifeEquipped::BeginPlay()
{
    Super::BeginPlay();
    
    BaseScale = KnifeMesh->GetRelativeScale3D();
    AnimInstance = Cast<UKayoKnifeAnim>(KnifeMesh->GetAnimInstance());
    
    // 나이프 글로우 이펙트 설정
    if (KnifeGlowEffect && GlowEffectComponent)
    {
        GlowEffectComponent->SetAsset(KnifeGlowEffect);
    }
    
    // Owner가 이미 설정되어 있으면 가시성 업데이트
    if (GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void AKayoKnifeEquipped::OnRep_Owner()
{
    Super::OnRep_Owner();
    
    // Owner가 변경되면 가시성 설정 업데이트
    UpdateVisibilitySettings();
}

void AKayoKnifeEquipped::SetKnifeViewType(EKnifeViewType ViewType)
{
    KnifeViewType = ViewType;
    
    // 서버에서 타입 변경
    if (HasAuthority())
    {
        OnRep_KnifeViewType();
    }
}

void AKayoKnifeEquipped::OnRep_KnifeViewType()
{
    UpdateVisibilitySettings();
}

void AKayoKnifeEquipped::UpdateVisibilitySettings()
{
    if (!GetOwner())
        return;
        
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
        return;
    
    // 로컬 플레이어 컨트롤러 가져오기
    APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
    bool bIsLocalOwner = LocalPC && LocalPC->GetPawn() == OwnerPawn;
    
    if (KnifeViewType == EKnifeViewType::FirstPerson)
    {
        // 1인칭 나이프 - 오직 오너만 볼 수 있음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 보이게 설정
            KnifeMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            
            // Owner만 보기 설정
            KnifeMesh->SetOnlyOwnerSee(true);
            GlowEffectComponent->SetOnlyOwnerSee(true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 숨김
            KnifeMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
        }
    }
    else  // ThirdPerson
    {
        // 3인칭 나이프 - 오너는 볼 수 없음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 숨김
            KnifeMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 보이게 설정
            KnifeMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            
            // Owner는 못보게 설정
            KnifeMesh->SetOwnerNoSee(true);
            GlowEffectComponent->SetOwnerNoSee(true);
        }
    }
    
    // 사운드 처리 (처음 한 번만)
    if (!bVisibilityInitialized && KnifeIdleSound && bIsEquipped)
    {
        bool bShouldPlaySound = (KnifeViewType == EKnifeViewType::FirstPerson && bIsLocalOwner) ||
                               (KnifeViewType == EKnifeViewType::ThirdPerson && !bIsLocalOwner);
        
        if (bShouldPlaySound)
        {
            IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
                KnifeIdleSound, 
                KnifeMesh, 
                NAME_None, 
                FVector::ZeroVector, 
                EAttachLocation::KeepRelativeOffset, 
                true
            );
        }
        
        bVisibilityInitialized = true;
    }
}

void AKayoKnifeEquipped::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 가시성이 설정되지 않았으면 재시도
    if (!bVisibilityInitialized && GetOwner())
    {
        UpdateVisibilitySettings();
    }
}

void AKayoKnifeEquipped::OnEquip()
{
    if (HasAuthority())
    {
        bIsEquipped = true;
        OnRep_IsEquipped();
        MulticastPlayEquipSound();
    }
    
    // 애니메이션 실행
    if (AnimInstance)
    {
        AnimInstance->OnKnifeEquip();
    }
}

void AKayoKnifeEquipped::OnUnequip()
{
    if (HasAuthority())
    {
        bIsEquipped = false;
        OnRep_IsEquipped();
        MulticastPlayUnequipSound();
    }
    
    // 애니메이션 실행 (필요시 구현)
    // if (AnimInstance)
    // {
    //     AnimInstance->OnKnifeUnequip();
    // }
}

void AKayoKnifeEquipped::OnRep_IsEquipped()
{
    UpdateEquipVisuals();
}

void AKayoKnifeEquipped::UpdateEquipVisuals()
{
    // 보이는 경우에만 장착 시각 효과 업데이트
    if (!KnifeMesh || !KnifeMesh->IsVisible())
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
    
    // 장착/해제 사운드 재생은 Multicast 함수에서 처리
}

void AKayoKnifeEquipped::MulticastPlayEquipSound_Implementation()
{
    if (KnifeEquipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), KnifeEquipSound, GetActorLocation());
    }
}

void AKayoKnifeEquipped::MulticastPlayUnequipSound_Implementation()
{
    if (KnifeUnequipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), KnifeUnequipSound, GetActorLocation());
    }
}

void AKayoKnifeEquipped::MulticastPlayIdleSound_Implementation()
{
    if (KnifeIdleSound && !IdleAudioComponent)
    {
        IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
            KnifeIdleSound, 
            KnifeMesh, 
            NAME_None, 
            FVector::ZeroVector, 
            EAttachLocation::KeepRelativeOffset, 
            true
        );
    }
}