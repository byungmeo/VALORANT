#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KayoGrenadeEquipped.generated.h"

UENUM(BlueprintType)
enum class EGrenadeViewType : uint8
{
    FirstPerson,
    ThirdPerson
};

UCLASS()
class VALORANT_API AKayoGrenadeEquipped : public AActor
{
    GENERATED_BODY()

public:
    AKayoGrenadeEquipped();

    // 수류탄 타입 설정 (1인칭/3인칭)
    UFUNCTION(BlueprintCallable, Category = "Kayo Grenade")
    void SetGrenadeViewType(EGrenadeViewType ViewType);

    // 장착 애니메이션
    UFUNCTION(BlueprintCallable, Category = "Kayo Grenade")
    void OnEquip();

    // 해제 애니메이션
    UFUNCTION(BlueprintCallable, Category = "Kayo Grenade")
    void OnUnequip();

    // 던지기 타입 설정 (언더핸드/오버핸드)
    UFUNCTION(BlueprintCallable, Category = "Kayo Grenade")
    void SetThrowType(bool bIsOverhand);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Owner가 복제될 때 호출
    virtual void OnRep_Owner() override;

    // 수류탄 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* GrenadeMesh;

    // 수류탄 설정
    UPROPERTY(EditDefaultsOnly, Category = "Grenade Settings")
    float GrenadePulseSpeed = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Grenade Settings")
    float GrenadePulseScale = 0.1f;

    // 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* GrenadeGlowEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* GlowEffectComponent;

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* GrenadeEquipSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* GrenadeUnequipSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* GrenadeIdleSound;

private:
    // 수류탄 타입 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_GrenadeViewType)
    EGrenadeViewType GrenadeViewType = EGrenadeViewType::ThirdPerson;
    
    UFUNCTION()
    void OnRep_GrenadeViewType();
    
    // 장착 상태 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_IsEquipped)
    bool bIsEquipped = false;
    
    UFUNCTION()
    void OnRep_IsEquipped();
    
    // 던지기 타입 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_IsOverhand)
    bool bIsOverhand = false;
    
    UFUNCTION()
    void OnRep_IsOverhand();
    
    float CurrentPulseTime = 0.0f;
    FVector BaseScale;

    UPROPERTY()
    class UAudioComponent* IdleAudioComponent;
    
    // 가시성 설정
    void UpdateVisibilitySettings();
    
    // 장착 상태 업데이트
    void UpdateEquipVisuals();
    
    // 가시성 초기화 완료 플래그
    bool bVisibilityInitialized = false;

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayEquipSound();
    
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayUnequipSound();
    
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayIdleSound();
};