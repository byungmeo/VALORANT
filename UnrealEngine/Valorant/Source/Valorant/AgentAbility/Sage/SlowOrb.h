#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "SlowOrb.generated.h"

class ABaseGround;

UCLASS()
class VALORANT_API ASlowOrb : public ABaseProjectile
{
	GENERATED_BODY()

public:
	ASlowOrb();
	
private:
	// Ref: https://valorant.fandom.com/wiki/Deployment_types#Projectile
	// Updated to match actual Valorant Sage Q specs
	const float Speed = 3000.0f;        // 30m/s
	const float Gravity = 1.0f;         // Normal gravity
	const bool bShouldBounce = false;   // No bounce, activates on impact
	const float EquipTime = 0.8f;
	const float UnequipTime = 0.7f;
	
public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<ABaseGround> BaseGroundClass = nullptr;

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
private:
	void SpawnSlowField(const FHitResult& ImpactResult);
};