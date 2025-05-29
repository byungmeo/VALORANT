#include "SlowOrb.h"

#include "AgentAbility/BaseGround.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ASlowOrb::ASlowOrb()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	
	Sphere->SetSphereRadius(15.0f);
	Mesh->SetRelativeScale3D(FVector(.3f));
	
	static ConstructorHelpers::FObjectFinder<UMaterial> FireballMaterial(TEXT("/Script/Engine.Material'/Engine/VREditor/LaserPointer/LaserPointerMaterial.LaserPointerMaterial'"));
	if (FireballMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, FireballMaterial.Object);
	}
	
	// Update projectile properties to match Valorant Sage Q
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = 0.0f;
	ProjectileMovement->Friction = 0.0f;
}

// Called when the game starts or when spawned
void ASlowOrb::BeginPlay()
{
	Super::BeginPlay();
	
	// Set up collision to trigger on hit
	Sphere->OnComponentHit.AddDynamic(this, &ASlowOrb::OnHit);
}

void ASlowOrb::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Spawn slow field on any surface hit
	SpawnSlowField(Hit);
	
	// Destroy the projectile
	Destroy();
}

void ASlowOrb::SpawnSlowField(const FHitResult& ImpactResult)
{
	if (!BaseGroundClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaseGroundClass is not set!"));
		return;
	}
	
	// Calculate spawn location
	FVector SpawnLocation = ImpactResult.ImpactPoint;
	
	// Check if we hit a floor surface (normal pointing up)
	if (ImpactResult.ImpactNormal.Z > 0.7f) // Floor surface
	{
		// Spawn directly at impact point for floor
		SpawnLocation = ImpactResult.ImpactPoint;
	}
	else // Wall or ceiling surface
	{
		// Trace down to find floor
		FHitResult FloorHit;
		FVector TraceStart = ImpactResult.ImpactPoint;
		FVector TraceEnd = TraceStart - FVector(0, 0, 10000.0f); // Trace 100m down
		
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(GetInstigator());
		
		if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			SpawnLocation = FloorHit.ImpactPoint;
		}
		else
		{
			// If no floor found, spawn at a reasonable distance below
			SpawnLocation = ImpactResult.ImpactPoint - FVector(0, 0, 300.0f);
		}
	}
	
	// Spawn the slow field
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Instigator = this->GetInstigator();
	
	GetWorld()->SpawnActor<ABaseGround>(BaseGroundClass, SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
}