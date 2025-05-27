#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseGround.h"
#include "StimBeaconGround.generated.h"

UCLASS()
class VALORANT_API AStimBeaconGround : public ABaseGround
{
	GENERATED_BODY()

public:
	AStimBeaconGround();

protected:
	virtual void BeginPlay() override;
    
	// BaseGround 오버라이드 - 버프 효과 적용
	virtual void ApplyGameEffect() override;
    
	// 시각 효과를 위한 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UDecalComponent* RangeDecal;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UParticleSystemComponent* BuffParticle;

private:
	// StimBeacon 설정값 (BaseGround의 const 변수들을 오버라이드)
	const float Radius = 600.0f;
	const float Duration = 12.0f;
	const float BuffRate = 0.5f; // 0.5초마다 버프 갱신
};