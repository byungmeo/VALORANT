#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "ResourceManager/ValorantGameType.h"
#include "NiagaraSystem.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "BaseGameplayAbility.generated.h"

class ABaseProjectile;
class UAgentAbilitySystemComponent;

UENUM(BlueprintType)
enum class EAbilityActivationType : uint8
{
    Instant,        // 즉시 실행
    WithPrepare     // 준비 -> 대기 -> 실행
};

UENUM(BlueprintType)
enum class EFollowUpInputType : uint8
{
    None,
    LeftClick,
    RightClick,
    LeftOrRight
};

UENUM(BlueprintType)
enum class EAbilityState : uint8
{
    None,
    Preparing,
    Waiting,
    Executing,
    Completed,
    Cancelled
};

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityStateChanged, FGameplayTag, StateTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPrepareAbility, FGameplayTag, SlotTag, EFollowUpInputType, InputType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndAbility);

UCLASS()
class VALORANT_API UBaseGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UBaseGameplayAbility();

    // 상태 관리
    UFUNCTION(BlueprintPure, Category = "Ability|State")
    EAbilityState GetCurrentState() const { return CurrentState; }

    UFUNCTION(BlueprintPure, Category = "Ability|State")
    bool IsInState(EAbilityState State) const { return CurrentState == State; }

    // 후속 입력 처리
    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void HandleFollowUpInput(FGameplayTag InputTag);

    // 설정
    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    int32 m_AbilityID = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    EAbilityActivationType ActivationType = EAbilityActivationType::Instant;

    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    EFollowUpInputType FollowUpInputType = EFollowUpInputType::None;

    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    float FollowUpTime = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
    bool bAllowCancelDuringExecution = false;

    // 애니메이션
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* PrepareMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* PrepareMontage_3P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* WaitingMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* WaitingMontage_3P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteMontage_3P = nullptr;
    
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteLeftMouseButtonMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteLeftMouseButtonMontage_3P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteRightMouseButtonMontage_1P = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* ExecuteRightMouseButtonMontage_3P = nullptr;
    
    // 투사체
    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    TSubclassOf<ABaseProjectile> ProjectileClass;

    // 효과
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* PrepareEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* PrepareSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* WaitEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* WaitSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    UNiagaraSystem* ExecuteEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    USoundBase* ExecuteSound = nullptr;

    // 델리게이트
    UPROPERTY(BlueprintAssignable)
    FOnAbilityStateChanged OnStateChanged;

    UPROPERTY()
    FOnPrepareAbility OnPrepareAbility;
    
    UPROPERTY()
    FOnEndAbility OnEndAbility;

protected:
    // GameplayAbility 오버라이드
    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
    
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo,
                                const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                           const FGameplayAbilityActorInfo* ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo,
                           bool bReplicateEndAbility, bool bWasCancelled) override;

    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                   const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayTagContainer* SourceTags = nullptr,
                                   const FGameplayTagContainer* TargetTags = nullptr,
                                   FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, 
                              const FGameplayAbilityActorInfo* ActorInfo,
                              const FGameplayAbilityActivationInfo ActivationInfo,
                              bool bReplicateCancelAbility) override;

    // 상태 전환
    UFUNCTION(BlueprintCallable, Category = "Ability|State")
    void TransitionToState(EAbilityState NewState);

    // 어빌리티 동작 - 서브클래스에서 오버라이드
    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void PrepareAbility();
    
    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void WaitAbility();
    
    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void ExecuteAbility();

    // 후속 입력 핸들러 - 서브클래스에서 오버라이드
    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual bool OnLeftClickInput();

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual bool OnRightClickInput();

    // 유틸리티
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ability")
    bool SpawnProjectile(FVector LocationOffset = FVector::ZeroVector, FRotator RotationOffset = FRotator::ZeroRotator);

    UFUNCTION(BlueprintCallable, Category = "Effects")
    void PlayCommonEffects(UNiagaraSystem* NiagaraEffect, USoundBase* SoundEffect, FVector Location = FVector::ZeroVector);

    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool ReduceAbilityStack();

    UFUNCTION(BlueprintPure, Category = "Ability")
    int32 GetAbilityStack() const;

    UPROPERTY(BlueprintReadWrite, Category = "Ability", EditAnywhere)
    UNiagaraSystem* ProjectileLaunchEffect;
    UPROPERTY(BlueprintReadWrite, Category = "Ability", EditAnywhere)
    USoundBase* ProjectileLaunchSound;

    // 애니메이션
    void PlayMontages(UAnimMontage* Montage1P, UAnimMontage* Montage3P, bool bStopAllMontages = true);
    void StopAllMontages();

    // 몽타주 콜백
    UFUNCTION()
    void OnMontageCompleted();

    UFUNCTION()
    void OnMontageBlendOut();

    // Actor를 통한 네트워크 동기화
    void NotifyStateChanged(EAbilityState NewState);
    void NotifyAbilityExecuted(bool bSuccess);

    // 캐시된 정보
    UPROPERTY()
    FGameplayAbilityActorInfo CachedActorInfo;

    UPROPERTY()
    UAgentAbilitySystemComponent* CachedASC = nullptr;

    UPROPERTY()
    ABaseProjectile* SpawnedProjectile = nullptr;

private:
    // 현재 상태
    UPROPERTY()
    EAbilityState CurrentState = EAbilityState::None;

    // 마지막 실행 입력 타입
    EFollowUpInputType LastExecuteInputType = EFollowUpInputType::None;

    // 이전 장비 상태
    UPROPERTY()
    EInteractorType PreviousEquipmentState = EInteractorType::None;

    // 타이머
    FTimerHandle WaitingTimeoutHandle;
    FTimerHandle CleanupDelayHandle;

    // 상태별 처리 함수
    void EnterState_Preparing();
    void EnterState_Waiting();
    void EnterState_Executing();
    void ExitCurrentState();

    // 정리 함수
    void CleanupAbility();
    void PerformFinalCleanup();

    // 타임아웃 핸들러
    UFUNCTION()
    void OnWaitingTimeout();

    // 헬퍼 함수
    FGameplayTag ConvertStateToTag(EAbilityState State) const;
    void UpdateAbilityTags(EAbilityState NewState);
    bool IsValidFollowUpInput(FGameplayTag InputTag) const;
};