#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseGround.generated.h"

class UGameplayEffect;
class ABaseAgent;

UCLASS()
class VALORANT_API ABaseGround : public AActor
{
	GENERATED_BODY()

public:
	ABaseGround();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TSet<ABaseAgent*> OverlappedAgents;
	
	FTimerHandle DamageTimerHandle;
	FTimerHandle DurationTimerHandle;

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void ApplyGameEffect();

	virtual void OnElapsedDuration();

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

protected:
	// Configurable parameters for different ground effects
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground Settings", meta = (DisplayName = "Ground Radius"))
	float Radius = 450.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground Settings", meta = (DisplayName = "Duration"))
	float Duration = 4.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground Settings", meta = (DisplayName = "Effect Apply Rate"))
	float DamageRate = 0.0167f;   // Apply effect ~60 times per second
};