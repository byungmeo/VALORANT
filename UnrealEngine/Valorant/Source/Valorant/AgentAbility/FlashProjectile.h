#pragma once

#include "CoreMinimal.h"
#include "AgentAbility/BaseProjectile.h"
#include "ResourceManager/ValorantGameType.h"
#include "FlashProjectile.generated.h"

class ABaseAgent;
class UParticleSystem;
class USoundBase;
class UNiagaraSystem;

UCLASS()
class VALORANT_API AFlashProjectile : public ABaseProjectile
{
    GENERATED_BODY()

public:
    AFlashProjectile();

protected:
    virtual void BeginPlay() override;
    virtual void OnProjectileBounced(const FHitResult& ImpactResult, const FVector& ImpactVelocity) override;

    // 섬광 폭발 처리
    UFUNCTION(BlueprintCallable, Category = "Flash")
    virtual void ExplodeFlash();

    // 플레이어가 섬광 범위에 있는지 체크 (거리 기반)
    bool IsPlayerInFlashRange(ABaseAgent* Player, float& OutBlindDuration);

    // 시야 차단 체크 (벽 뒤에 있으면 섬광 안됨)
    bool HasLineOfSight(ABaseAgent* Player);

    // 각 클라이언트에서 시야 각도 체크하도록 RPC 호출 - 섬광 위치 정보 추가
    UFUNCTION(NetMulticast, Reliable)
    void MulticastApplyFlashEffect(float BlindDuration, FVector FlashLocation, EFlashType InFlashType);
    
    // 섬광 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float FlashRadius = 2000.0f;

    // 최대/최소 완전 실명 시간
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float MaxBlindDuration = 4.0f;  // 최대 4초 완전 실명

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float MinBlindDuration = 1.0f;  // 최소 1초 완전 실명

    // 회복 시간 (항상 고정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float RecoveryDuration = 0.5f;  // 항상 0.5초 회복

    // 시야 각도 체크 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float ViewAngleThreshold = 90.0f;  // 90도 이내에 있어야 효과 적용

    // 폭발 대기 시간
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float DetonationDelay = 0.5f;

    // 섬광 타입
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    EFlashType FlashType = EFlashType::Default;

    // 최소 섬광 효과 설정 (각도로 인해 지속시간이 0이어도 적용)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float MinimumFlashDuration = 0.2f;  // 최소 0.2초

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash Settings", meta = (AllowPrivateAccess = "true"))
    float MinimumFlashIntensity = 0.8f;  // 최소 80% 강도

    // 이펙트 및 사운드
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    UParticleSystem* ExplosionEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    USoundBase* ExplosionSound;

    // 섬광에 걸린 대상 머리에 표시할 VFX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (AllowPrivateAccess = "true"))
    UNiagaraSystem* FlashedTargetVFX;

    bool bIsScheduledToExplode = false;
};