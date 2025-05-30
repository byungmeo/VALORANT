#pragma once

#include "CoreMinimal.h"
#include "ValorantObject/HandChargeActor/BaseOrbActor.h"
#include "PhoenixFireOrbActor.generated.h"

UCLASS()
class VALORANT_API APhoenixFireOrbActor : public ABaseOrbActor
{
	GENERATED_BODY()

public:
	APhoenixFireOrbActor();

	// 던지기 타입 설정 (직선/포물선 시각적 표시용)
	UFUNCTION(BlueprintCallable, Category = "Phoenix Fire Orb")
	void SetThrowType(bool bIsCurved);

protected:
	virtual void BeginPlay() override;
	virtual void UpdateCustomAnimation(float DeltaTime) override;

	// 화염 색상들
	UPROPERTY(EditDefaultsOnly, Category = "Fire Colors")
	FLinearColor StraightFireColor = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f);  // 주황색

	UPROPERTY(EditDefaultsOnly, Category = "Fire Colors")
	FLinearColor CurvedFireColor = FLinearColor(1.0f, 0.6f, 0.0f, 1.0f);    // 노란 주황색

	// 화염 이펙트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UNiagaraComponent* FlameEffect;

private:
	bool bIsCurvedThrow = false;
    
	// 화염 효과를 위한 애니메이션 변수
	float FlameIntensityTime = 0.0f;
    
	void UpdateFlameEffects();
};