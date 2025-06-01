#include "Phoenix_E_EquippedCurveball.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"

APhoenix_E_EquippedCurveball::APhoenix_E_EquippedCurveball()
{
    bReplicates = true;
    SetReplicateMovement(true);

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    CurveballMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CurveballMesh"));
    CurveballMesh->SetupAttachment(GetRootComponent());
    CurveballMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CurveballMesh->SetIsReplicated(true);
    
    GlowEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GlowEffect"));
    GlowEffectComponent->SetupAttachment(CurveballMesh);
    GlowEffectComponent->bAutoActivate = false;
    GlowEffectComponent->SetIsReplicated(true);
}

void APhoenix_E_EquippedCurveball::BeginPlay()
{
    Super::BeginPlay();
    UpdateVisibilitySettings();
}

void APhoenix_E_EquippedCurveball::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APhoenix_E_EquippedCurveball, CurveballViewType);
}

void APhoenix_E_EquippedCurveball::SetCurveballViewType(EViewType ViewType)
{
    CurveballViewType = ViewType;
    if (HasAuthority())
        OnRep_CurveballViewType();
}

void APhoenix_E_EquippedCurveball::OnRep_CurveballViewType()
{
    UpdateVisibilitySettings();
}

void APhoenix_E_EquippedCurveball::UpdateVisibilitySettings()
{
    
    if (!GetOwner())
        return;
        
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
        return;
    
    // 로컬 플레이어 컨트롤러 가져오기
    APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
    bool bIsLocalOwner = LocalPC && LocalPC->GetPawn() == OwnerPawn;
    
    if (CurveballViewType == EViewType::FirstPerson)
    {
        // 1인칭 수류탄 - 오직 오너만 볼 수 있음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 보이게 설정
            CurveballMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            
            // Owner만 보기 설정
            CurveballMesh->SetOnlyOwnerSee(true);
            GlowEffectComponent->SetOnlyOwnerSee(true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 숨김
            CurveballMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
        }
    }
    else  // ThirdPerson
    {
        // 3인칭 수류탄 - 오너는 볼 수 없음
        if (bIsLocalOwner)
        {
            // 로컬 오너인 경우 - 숨김
            CurveballMesh->SetVisibility(false, true);
            GlowEffectComponent->SetVisibility(false, true);
        }
        else
        {
            // 로컬 오너가 아닌 경우 - 보이게 설정
            CurveballMesh->SetVisibility(true, true);
            GlowEffectComponent->SetVisibility(true, true);
            
            // Owner는 못보게 설정
            CurveballMesh->SetOwnerNoSee(true);
            GlowEffectComponent->SetOwnerNoSee(true);
        }
    }
}

void APhoenix_E_EquippedCurveball::OnEquip()
{
    // 이펙트/사운드 등
    if (EquipSound)
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), EquipSound, GetActorLocation());
    if (GlowEffectComponent)
        GlowEffectComponent->Activate();
    
}

void APhoenix_E_EquippedCurveball::OnUnequip()
{
    if (UnequipSound)
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), UnequipSound, GetActorLocation());
    if (GlowEffectComponent)
        GlowEffectComponent->Deactivate();
}