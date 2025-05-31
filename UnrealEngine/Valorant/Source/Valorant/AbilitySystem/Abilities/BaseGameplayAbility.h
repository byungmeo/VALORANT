#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "ResourceManager/ValorantGameType.h"
#include "NiagaraSystem.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "BaseGameplayAbility.generated.h"

class ABaseProjectile;

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
	RepeatKey,
	LeftOrRight,    // 좌클릭 또는 우클릭
	Any             // 모든 후속 입력
};

// 어빌리티 상태를 명확하게 정의
UENUM(BlueprintType)
enum class EAbilityState : uint8
{
	None,
	Preparing,
	Waiting,
	Executing,
	Canceling,
	Ending
};

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityStateChanged, EAbilityState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPrepareAbility, FGameplayTag, SlotTag, EFollowUpInputType, InputType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndAbility);

UCLASS()
class VALORANT_API UBaseGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBaseGameplayAbility();

	// === 상태 관리 ===
	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	EAbilityState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	bool IsInState(EAbilityState State) const { return CurrentState == State; }

	// === Utility ===
	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	bool bAllowCancelDuringExecution = false;
	
	virtual bool CanBeCanceled() const override;

	// === 설정 ===
	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	int32 m_AbilityID = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	EAbilityActivationType ActivationType = EAbilityActivationType::Instant;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	EFollowUpInputType FollowUpInputType = EFollowUpInputType::None;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Config")
	float FollowUpTime = 10.0f;

	// === 애니메이션 (1인칭/3인칭 페어) ===
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

	// === 투사체 ===
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class ABaseProjectile> ProjectileClass;

	// === 공통 효과 ===
	UPROPERTY(EditDefaultsOnly, Category = "Effects|Projectile")
	class UNiagaraSystem* ProjectileLaunchEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Projectile")
	class USoundBase* ProjectileLaunchSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Prepare")
	class UNiagaraSystem* PrepareEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Prepare")
	class USoundBase* PrepareSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Execute")
	class UNiagaraSystem* ExecuteEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Execute")
	class USoundBase* ExecuteSound = nullptr;

	// === 델리게이트 ===
	UPROPERTY(BlueprintAssignable)
	FOnAbilityStateChanged OnStateChanged;

	UPROPERTY()
	FOnPrepareAbility OnPrepareAbility;
	
	UPROPERTY()
	FOnEndAbility OnEndAbility;

	// === 후속 입력 처리 ===
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void HandleFollowUpInput(FGameplayTag InputTag);

protected:
	// === GameplayAbility 오버라이드 ===
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

	// === 상태 전환 ===
	UFUNCTION(BlueprintCallable, Category = "Ability|State")
	void TransitionToState(EAbilityState NewState);

	// 상태별 진입 함수
	virtual void EnterState_Preparing();
	virtual void EnterState_Waiting();
	virtual void EnterState_Executing();
	virtual void EnterState_Canceling();
	virtual void EnterState_Ending();

	// 상태별 퇴출 함수
	virtual void ExitState_Preparing();
	virtual void ExitState_Waiting();
	virtual void ExitState_Executing();

	// === 어빌리티 동작 로직 ===
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual void PrepareAbility();
	
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual void WaitAbility();
	
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual void ExecuteAbility();

	// === 후속 입력 처리 ===
	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual bool OnLeftClickInput();

	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual bool OnRightClickInput();

	UFUNCTION(BlueprintCallable, Category = "Ability")
	virtual bool OnRepeatInput();

	// === 애니메이션 ===
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayMontages(UAnimMontage* Montage1P, UAnimMontage* Montage3P, bool bStopAllMontages = true);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	// === 유틸리티 ===
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ability")
	bool SpawnProjectile(FVector LocationOffset = FVector::ZeroVector, FRotator RotationOffset = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "Effects")
	void PlayCommonEffects(class UNiagaraSystem* NiagaraEffect, class USoundBase* SoundEffect, FVector Location = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool ReduceAbilityStack();

	UFUNCTION(BlueprintPure, Category = "Ability")
	int32 GetAbilityStack() const;

	UFUNCTION()
	void StopAllMontages() const;

	// === 공통 정리 함수 ===
	void CleanupAbility();

	UPROPERTY()
	ABaseProjectile* SpawnedProjectile = nullptr;

	UPROPERTY()
	FGameplayAbilityActorInfo CachedActorInfo;

	UPROPERTY()
	EInteractorType PreviousEquipmentState = EInteractorType::None;

private:
	// 현재 상태
	UPROPERTY()
	EAbilityState CurrentState = EAbilityState::None;

	// 타이머 핸들
	FTimerHandle WaitingTimeoutHandle;

	// 후속 입력 관리
	void RegisterFollowUpInputs();
	void UnregisterFollowUpInputs();

	UFUNCTION()
	void OnWaitingTimeout();

	static FGameplayTag ConvertAbilityStateToGamePlayTag(EAbilityState NewState);
};