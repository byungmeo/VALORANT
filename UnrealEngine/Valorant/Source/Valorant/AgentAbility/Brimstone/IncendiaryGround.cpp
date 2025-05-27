#include "IncendiaryGround.h"

#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/Agent/BaseAgent.h"
#include "Sound/SoundCue.h"


AIncendiaryGround::AIncendiaryGround()
{
	// 기본 메시 설정
	GroundMesh->SetVisibility(false);
	const float Scale = Radius * 2.f / 100.f;
	GroundMesh->SetRelativeScale3D(FVector(Scale, Scale, 0.5f));
    
	// 불 데칼
	FireDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("FireDecal"));
	FireDecal->SetupAttachment(RootComponent);
	FireDecal->DecalSize = FVector(Radius, Radius, 100.0f);
	FireDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    
	// 불 파티클
	FireParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireParticle"));
	FireParticle->SetupAttachment(RootComponent);
    
	// 불 소리
	FireAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudio"));
	FireAudio->SetupAttachment(RootComponent);
	FireAudio->bAutoActivate = true;
    
	// 머티리얼 설정
	static ConstructorHelpers::FObjectFinder<UMaterial> FireDecalMaterial(TEXT("/Script/Engine.Material'/Game/Materials/M_FireGround.M_FireGround'"));
	if (FireDecalMaterial.Succeeded())
	{
		FireDecal->SetDecalMaterial(FireDecalMaterial.Object);
	}
    
	// 파티클 설정
	static ConstructorHelpers::FObjectFinder<UParticleSystem> FireParticleAsset(TEXT("/Script/Engine.ParticleSystem'/Game/Effects/Fire/P_FireGround.P_FireGround'"));
	if (FireParticleAsset.Succeeded())
	{
		FireParticle->SetTemplate(FireParticleAsset.Object);
	}
    
	// 사운드 설정
	static ConstructorHelpers::FObjectFinder<USoundCue> FireSoundCue(TEXT("/Script/Engine.SoundCue'/Game/Audio/SFX/Fire/Cue_FireGround_Loop.Cue_FireGround_Loop'"));
	if (FireSoundCue.Succeeded())
	{
		FireAudio->SetSound(FireSoundCue.Object);
	}
}

void AIncendiaryGround::BeginPlay()
{
	Super::BeginPlay();
    
	// 시작 시 폭발 효과
	if (HasAuthority())
	{
		// 폭발 파티클 재생
		UParticleSystem* ExplosionParticle = LoadObject<UParticleSystem>(nullptr, 
			TEXT("/Script/Engine.ParticleSystem'/Game/Effects/Fire/P_FireExplosion.P_FireExplosion'"));
		if (ExplosionParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticle, 
				GetActorLocation(), FRotator::ZeroRotator, FVector(1.5f));
		}
        
		// 폭발 사운드 재생
		USoundCue* ExplosionSound = LoadObject<USoundCue>(nullptr, 
			TEXT("/Script/Engine.SoundCue'/Game/Audio/SFX/Fire/Cue_FireExplosion.Cue_FireExplosion'"));
		if (ExplosionSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());
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
    
	// 팀 체크 (같은 팀이면 데미지 없음)
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(GetInstigator());
	if (OwnerAgent)
	{
		if (Agent->IsBlueTeam() == OwnerAgent->IsBlueTeam())
		{
			return;
		}
	}
    
	// 데미지 적용
	Agent->ServerApplyGE(GameplayEffect, OwnerAgent);
}
