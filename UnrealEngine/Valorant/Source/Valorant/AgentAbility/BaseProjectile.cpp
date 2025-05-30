// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseProjectile.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ABaseProjectile::ABaseProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;
	AActor::SetReplicateMovement(true);
	
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Sphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);     // 벽, 바닥 등
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);    // 박스, 오브젝트 등
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);           // 플레이어 무시
	SetRootComponent(Sphere);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->ProjectileGravityScale = 2.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.5f;
	ProjectileMovement->bAutoActivate = true;
}

// Called when the game starts or when spawned
void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();
	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &ABaseProjectile::OnProjectileBounced);
}

// Called every frame
void ABaseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseProjectile::OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	//
}

void ABaseProjectile::NetMulti_WarningEffects_Implementation(FVector Location)
{
	if (WarningVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WarningVFX, Location);
	}
	if (WarningSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), WarningSFX, Location);
	}
}

void ABaseProjectile::NetMulti_ExplosionEffects_Implementation(FVector Location)
{
	if (ExplosionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionVFX, Location);
	}
	if (ExplosionSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSFX, Location);
	}
}
