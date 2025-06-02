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
	// StimBeacon 설정값 (발로란트 실제 수치)
	const float Radius = 1200.0f; // 12미터
	const float Duration = 12.0f; // 12초 지속
	const float BuffRate = 0.5f; // 0.5초마다 버프 갱신
};