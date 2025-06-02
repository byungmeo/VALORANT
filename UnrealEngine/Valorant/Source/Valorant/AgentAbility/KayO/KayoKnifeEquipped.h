#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KayoKnifeEquipped.generated.h"

UENUM(BlueprintType)
enum class EKnifeViewType : uint8
{
    FirstPerson,
    ThirdPerson
};

UCLASS()
class VALORANT_API AKayoKnifeEquipped : public AActor
{
    GENERATED_BODY()

public:
    AKayoKnifeEquipped();

    // 나이프 타입 설정 (1인칭/3인칭)
    UFUNCTION(BlueprintCallable, Category = "Kayo Knife")
    void SetKnifeViewType(EKnifeViewType ViewType);

    // 장착 애니메이션
    UFUNCTION(BlueprintCallable, Category = "Kayo Knife")
    void OnEquip();

    // 해제 애니메이션
    UFUNCTION(BlueprintCallable, Category = "Kayo Knife")
    void OnUnequip();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Owner가 복제될 때 호출
    virtual void OnRep_Owner() override;

    // 나이프 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USkeletalMeshComponent* KnifeMesh;

    // 나이프 애니메이션 인스턴스
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    class UKayoKnifeAnim* AnimInstance;

    // 나이프 설정
    UPROPERTY(EditDefaultsOnly, Category = "Knife Settings")
    float KnifeIdleSpeed = 30.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Knife Settings")
    float KnifePulseSpeed = 1.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Knife Settings")
    float KnifePulseScale = 0.05f;

    // 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* KnifeGlowEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* GlowEffectComponent;

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* KnifeEquipSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* KnifeUnequipSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* KnifeIdleSound;

private:
    // 나이프 타입 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_KnifeViewType)
    EKnifeViewType KnifeViewType = EKnifeViewType::ThirdPerson;
    
    UFUNCTION()
    void OnRep_KnifeViewType();
    
    // 장착 상태 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_IsEquipped)
    bool bIsEquipped = false;
    
    UFUNCTION()
    void OnRep_IsEquipped();
    
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