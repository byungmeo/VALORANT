#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KayoFlashbangEquipped.generated.h"

UENUM(BlueprintType)
enum class EFlashbangViewType : uint8
{
    FirstPerson,
    ThirdPerson
};

UCLASS()
class VALORANT_API AKayoFlashbangEquipped : public AActor
{
    GENERATED_BODY()

public:
    AKayoFlashbangEquipped();

    // 플래시뱅 타입 설정 (1인칭/3인칭)
    UFUNCTION(BlueprintCallable, Category = "Kayo Flashbang")
    void SetFlashbangViewType(EFlashbangViewType ViewType);

    // 장착 애니메이션
    UFUNCTION(BlueprintCallable, Category = "Kayo Flashbang")
    void OnEquip();

    // 해제 애니메이션
    UFUNCTION(BlueprintCallable, Category = "Kayo Flashbang")
    void OnUnequip();

    // 던지기 타입 미리보기 (직선/포물선)
    UFUNCTION(BlueprintCallable, Category = "Kayo Flashbang")
    void SetThrowPreview(bool bIsAltFire);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Owner가 복제될 때 호출
    virtual void OnRep_Owner() override;

    // 플래시뱅 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USkeletalMeshComponent* FlashbangMesh;

    // 플래시뱅 설정
    UPROPERTY(EditDefaultsOnly, Category = "Flashbang Settings")
    float FlashbangRotationSpeed = 60.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Flashbang Settings")
    float FlashbangPulseSpeed = 2.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Flashbang Settings")
    float FlashbangPulseScale = 0.08f;

    // 이펙트
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* FlashbangGlowEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* GlowEffectComponent;

    // 궤적 미리보기 이펙트 (선택사항)
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* TrajectoryPreviewEffect;

    // 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* FlashbangEquipSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* FlashbangUnequipSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sounds")
    class USoundBase* FlashbangIdleSound;

private:
    // 플래시뱅 타입 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_FlashbangViewType)
    EFlashbangViewType FlashbangViewType = EFlashbangViewType::ThirdPerson;
    
    UFUNCTION()
    void OnRep_FlashbangViewType();
    
    // 장착 상태 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_IsEquipped)
    bool bIsEquipped = false;
    
    UFUNCTION()
    void OnRep_IsEquipped();
    
    // 던지기 미리보기 타입 (복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_IsAltFire)
    bool bIsAltFire = false;
    
    UFUNCTION()
    void OnRep_IsAltFire();
    
    float CurrentPulseTime = 0.0f;
    float CurrentRotation = 0.0f;
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