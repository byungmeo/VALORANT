#include "StimBeaconGround.h"
#include "Components/DecalComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

AStimBeaconGround::AStimBeaconGround()
{
    // 기본 메시는 숨기고 데칼로 범위 표시
    GroundMesh->SetVisibility(false);
    GroundMesh->SetCollisionProfileName("OverlapOnlyPawn");
    
    // 범위 표시용 데칼
    RangeDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("RangeDecal"));
    RangeDecal->SetupAttachment(RootComponent);
    RangeDecal->DecalSize = FVector(Radius, Radius, 100.0f);
    RangeDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    
    // 버프 파티클 효과
    BuffParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BuffParticle"));
    BuffParticle->SetupAttachment(RootComponent);
}

void AStimBeaconGround::BeginPlay()
{
    Super::BeginPlay();
    
    // 콜리전 범위 설정
    const float Scale = Radius * 2.f / 100.f;
    GroundMesh->SetRelativeScale3D(FVector(Scale, Scale, 10.0f)); // 높이를 크게 설정하여 점프해도 버프 받도록
}

void AStimBeaconGround::ApplyGameEffect()
{
    if (IsActorBeingDestroyed())
    {
        return;
    }
    
    // 팀 체크를 위한 Instigator 가져오기
    ABaseAgent* OwnerAgent = Cast<ABaseAgent>(GetInstigator());
    if (!OwnerAgent)
    {
        return;
    }
    
    // 범위 내 모든 에이전트에게 버프 적용
    for (auto* Agent : OverlappedAgents)
    {
        if (!Agent)
        {
            continue;
        }
        
        // 같은 팀인지 확인 
        if (Agent->IsBlueTeam() == OwnerAgent->IsBlueTeam())
        {
            // GameplayEffect로 버프 적용
            if (GameplayEffect)
            {
                Agent->ServerApplyGE(GameplayEffect, OwnerAgent);
            }
            
            UE_LOG(LogTemp, Warning, TEXT("StimBeacon: %s에게 버프 적용"), *Agent->GetName());
        }
    }
}