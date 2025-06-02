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
	
	// 메시 에셋 설정 (구체)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
	
	Sphere->SetSphereRadius(15.0f);
	Mesh->SetRelativeScale3D(FVector(.3f));
	
	// 머티리얼 설정 (세이지 슬로우 오브용)
	static ConstructorHelpers::FObjectFinder<UMaterial> SlowOrbMaterial(TEXT("/Script/Engine.Material'/Engine/VREditor/LaserPointer/LaserPointerMaterial.LaserPointerMaterial'"));
	if (SlowOrbMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, SlowOrbMaterial.Object);
	}
	
	// Update projectile properties to match Valorant Sage Q
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	ProjectileMovement->bShouldBounce = bShouldBounce;
	ProjectileMovement->Bounciness = 0.2f;
	ProjectileMovement->Friction = 0.8f;
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
	// 실제 발로란트 동작: 바닥에 부딪힐 때만 슬로우 필드 생성
	if (Hit.ImpactNormal.Z > 0.7f) // 바닥 표면 (노멀이 위를 향함)
	{
		// 바닥에 슬로우 필드 생성
		SpawnSlowField(Hit);
		
		// 폭발 효과
		if (HasAuthority())
		{
			NetMulti_ExplosionEffects(Hit.ImpactPoint);
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Sage SlowOrb - Hit floor, spawning slow field"));
	}
	else
	{
		return;
	}
	
	// 오브 파괴
	Destroy();
}

void ASlowOrb::SpawnSlowField(const FHitResult& ImpactResult)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (!BaseGroundClass)
	{
		UE_LOG(LogTemp, Error, TEXT("BaseGroundClass is not set!"));
		return;
	}
	
	// 바닥에 직접 스폰
	FVector SpawnLocation = ImpactResult.ImpactPoint;
	
	// 스폰 파라미터 설정
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.Owner = GetOwner();
	
	// 슬로우 필드 스폰
	ABaseGround* SlowField = GetWorld()->SpawnActor<ABaseGround>(BaseGroundClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	
	if (SlowField)
	{
		UE_LOG(LogTemp, Warning, TEXT("Sage SlowOrb - Slow field spawned at %s"), *SpawnLocation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Sage SlowOrb - Failed to spawn slow field"));
	}
}

void ASlowOrb::NetMulti_DestroyEffects_Implementation(const FVector& Location)
{
	// 벽/천장 충돌 시 파괴 효과 (슬로우 필드 없이)
	if (DestroyEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DestroyEffect, Location);
	}
	
	if (DestroySound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DestroySound, Location);
	}
}