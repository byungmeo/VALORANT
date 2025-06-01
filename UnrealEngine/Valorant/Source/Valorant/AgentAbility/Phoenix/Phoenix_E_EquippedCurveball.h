#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceManager/ValorantGameType.h"
#include "Phoenix_E_EquippedCurveball.generated.h"

UCLASS()
class VALORANT_API APhoenix_E_EquippedCurveball : public AActor
{
	GENERATED_BODY()

public:
	APhoenix_E_EquippedCurveball();

	UFUNCTION(BlueprintCallable)
	void SetCurveballViewType(EViewType ViewType);

	UFUNCTION(BlueprintCallable)
	void OnEquip();

	UFUNCTION(BlueprintCallable)
	void OnUnequip();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* CurveballMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UNiagaraSystem* GlowEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UNiagaraComponent* GlowEffectComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	class USoundBase* EquipSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sounds")
	class USoundBase* UnequipSound;

private:
	UPROPERTY(ReplicatedUsing = OnRep_CurveballViewType)
	EViewType CurveballViewType = EViewType::ThirdPerson;

	UFUNCTION()
	void OnRep_CurveballViewType();

	void UpdateVisibilitySettings();
};