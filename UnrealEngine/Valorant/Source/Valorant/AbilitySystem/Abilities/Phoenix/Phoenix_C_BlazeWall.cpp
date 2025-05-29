#include "Phoenix_C_BlazeWall.h"
#include "Player/Agent/BaseAgent.h"
#include "Components/BoxComponent.h"
#include "Player/AgentPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"

APhoenix_C_BlazeWall::APhoenix_C_BlazeWall()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // BoxComponent를 새로운 루트로 설정
    WallCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WallCollision"));
    SetRootComponent(WallCollision);
    WallCollision->SetBoxExtent(FVector(WallWidth / 2.0f, 50.0f, WallHeight / 2.0f));
    WallCollision->SetCollisionProfileName("OverlapOnlyPawn");
    
    // 기존 GroundMesh를 벽 형태로 재설정
    if (GroundMesh)
    {
        GroundMesh->SetupAttachment(WallCollision);
        GroundMesh->SetRelativeLocation(FVector(0, 0, 0));
        GroundMesh->SetRelativeScale3D(FVector(WallWidth / 100.0f, 1.0f, WallHeight / 100.0f));
        
        // 박스 메시 사용
        static ConstructorHelpers::FObjectFinder<UStaticMesh> WallMeshAsset(
            TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
        if (WallMeshAsset.Succeeded())
        {
            GroundMesh->SetStaticMesh(WallMeshAsset.Object);
        }
        
        // 불 효과를 위한 반투명 머티리얼 (임시)
        static ConstructorHelpers::FObjectFinder<UMaterialInstance> FireMaterial(
            TEXT("/Script/Engine.MaterialInstanceConstant'/Engine/VREditor/LaserPointer/TranslucentLaserPointerMaterialInst.TranslucentLaserPointerMaterialInst'"));
        if (FireMaterial.Succeeded())
        {
            GroundMesh->SetMaterial(0, FireMaterial.Object);
        }
        
        GroundMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    // 충돌 이벤트는 WallCollision에서 처리
    bReplicates = true;
    SetReplicateMovement(true);
    
    CalculateTickValues();
}

void APhoenix_C_BlazeWall::BeginPlay()
{
    // BaseGround의 BeginPlay를 호출하지 않고 직접 구현
    Super::Super::BeginPlay();  // AActor::BeginPlay() 호출
    
    InitializeWallSegment();
    
    if (HasAuthority())
    {
        // WallCollision에 충돌 이벤트 바인딩
        WallCollision->OnComponentBeginOverlap.AddDynamic(this, &APhoenix_C_BlazeWall::OnBeginOverlap);
        WallCollision->OnComponentEndOverlap.AddDynamic(this, &APhoenix_C_BlazeWall::OnEndOverlap);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Phoenix C Blaze Wall Created - Instigator: %s"), 
        (GetInstigator() ? *GetInstigator()->GetName() : TEXT("NULL")));
}

void APhoenix_C_BlazeWall::InitializeWallSegment()
{
    // 벽 지속 시간 설정
    GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle, this, 
        &APhoenix_C_BlazeWall::OnElapsedDuration, WallDuration, false);
}

void APhoenix_C_BlazeWall::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (IsActorBeingDestroyed() || !OtherActor || OtherActor == this)
    {
        return;
    }

    if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
    {
        // 이미 오버랩 중인 에이전트인지 확인
        if (!OverlappedAgents.Contains(Agent))
        {
            OverlappedAgents.Add(Agent);
            
            // 첫 번째 에이전트가 들어왔을 때 타이머 시작
            if (OverlappedAgents.Num() == 1 && !GetWorld()->GetTimerManager().IsTimerActive(DamageTimerHandle))
            {
                // 초당 4번 효과 적용 (0.25초마다)
                GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this, 
                    &APhoenix_C_BlazeWall::ApplyGameEffect, EffectApplicationInterval, true);
                
                // 즉시 첫 효과 적용
                ApplyGameEffect();
            }
        }
    }
}

void APhoenix_C_BlazeWall::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (IsActorBeingDestroyed() || !OtherActor)
    {
        return;
    }
    
    if (ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor))
    {
        OverlappedAgents.Remove(Agent);
        
        // 모든 에이전트가 벗어났을 때 타이머 정지
        if (OverlappedAgents.Num() == 0)
        {
            GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
        }
    }
}

void APhoenix_C_BlazeWall::ApplyGameEffect()
{
    if (IsActorBeingDestroyed())
    {
        return;
    }
    
    // 오버랩된 에이전트들을 복사 (반복 중 컬렉션 변경 방지)
    TArray<ABaseAgent*> AgentsCopy = OverlappedAgents.Array();
    
    for (ABaseAgent* Agent : AgentsCopy)
    {
        if (!IsValid(Agent))
        {
            OverlappedAgents.Remove(Agent);
            continue;
        }

        Agent->AdjustFlashEffectDirect(0.25f,0.25f);
        
        // Phoenix인지 확인
        bool bIsPhoenix = IsPhoenixOrAlly(Agent);
            
        if (bIsPhoenix)
        {
            // Phoenix에게 힐 적용
            if (GameplayEffect)
            {
                Agent->ServerApplyHealthGE(GameplayEffect, HealPerTick);
            }
        }
        else
        {
            // 적에게 데미지 적용
            if (GameplayEffect)
            {
                Agent->ServerApplyHealthGE(GameplayEffect, DamagePerTick);
            }
        }
    }
}

bool APhoenix_C_BlazeWall::IsPhoenixOrAlly(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }
    
    // 1. Instigator와 같은 에이전트인지 확인
    if (Actor == GetInstigator())
    {
        return true;
    }
    
    // 2. 같은 팀인지 확인
    if (ABaseAgent* Agent = Cast<ABaseAgent>(Actor))
    {
        if (ABaseAgent* InstigatorAgent = Cast<ABaseAgent>(GetInstigator()))
        {
            return InstigatorAgent->IsBlueTeam() == Agent->IsBlueTeam();
        }
    }
    
    return false;
}

void APhoenix_C_BlazeWall::OnElapsedDuration()
{
    if (!HasAuthority())
    {
        return;
    }
    
    // 타이머 정리
    GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);
    
    // 벽 제거
    Destroy();
}