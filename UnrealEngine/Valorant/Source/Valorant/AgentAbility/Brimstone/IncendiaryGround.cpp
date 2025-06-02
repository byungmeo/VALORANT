#include "IncendiaryGround.h"

#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "Sound/SoundCue.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"


AIncendiaryGround::AIncendiaryGround()
{
	// 기본 메시 설정
	GroundMesh->SetVisibility(false);
	const float Scale = Radius * 2.f / 100.f;
	GroundMesh->SetRelativeScale3D(FVector(Scale, Scale, 0.5f));
    
	// 나이아가라 루프 이펙트
	NiagaraLoop = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraLoop"));
	NiagaraLoop->SetupAttachment(RootComponent);
	NiagaraLoop->bAutoActivate = true;
    
	// 나이아가라 폭발 이펙트
	NiagaraExplosion = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraExplosion"));
	NiagaraExplosion->SetupAttachment(RootComponent);
	NiagaraExplosion->bAutoActivate = false;
    
	// 불 소리
	FireAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudio"));
	FireAudio->SetupAttachment(RootComponent);
	FireAudio->bAutoActivate = true;
}

void AIncendiaryGround::BeginPlay()
{
	Super::BeginPlay();
    
	if (HasAuthority())
	{
		// 폭발 이펙트 재생 (한 번만)
		if (NiagaraExplosion)
		{
			NiagaraExplosion->Activate(true);
		}
	}
}

void AIncendiaryGround::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
    
	if (IsActorBeingDestroyed() || !HasAuthority())
	{
		return;
	}
    
	ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor);
	if (!Agent)
	{
		return;
	}

	Agent->ServerApplyGE(GameplayEffect, Cast<ABaseAgent>(Owner));
}