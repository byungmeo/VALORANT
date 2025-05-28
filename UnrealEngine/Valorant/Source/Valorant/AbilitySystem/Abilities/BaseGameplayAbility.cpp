﻿#include "BaseGameplayAbility.h"

#include <GameManager/SubsystemSteamManager.h>

#include "Valorant.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AgentAbility/BaseProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerState.h"
#include "Player/Agent/BaseAgent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameplayTagAssetInterface.h"
#include "Net/UnrealNetwork.h"
#include "ValorantObject/BaseInteractor.h"

UBaseGameplayAbility::UBaseGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bReplicateInputDirectly = true;

	// 기본 태그 설정
	ActivationBlockedTags.AddTag(FValorantGameplayTags::Get().Block_Ability_Activation);
}

void UBaseGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	ABaseAgent* Agent = Cast<ABaseAgent>(ActorInfo->AvatarActor.Get());
	if (Agent)
	{
		OnPrepareAbility.AddDynamic(Agent, &ABaseAgent::OnAbilityPrepare);
		OnEndAbility.AddDynamic(Agent, &ABaseAgent::OnAbilityEnd);
	}
}

bool UBaseGameplayAbility::CanBeCanceled() const
{
	// 실행 단계에서는 기본적으로 취소 불가
	if (CurrentPhase == FValorantGameplayTags::Get().State_Ability_Executing)
	{
		return bAllowCancelDuringExecution;
	}
    
	// 준비나 대기 단계에서는 취소 가능
	return Super::CanBeCanceled();
}

bool UBaseGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayTagContainer* SourceTags,
                                              const FGameplayTagContainer* TargetTags,
                                              FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 차지 확인
	return GetAbilityStack() > 0;
}

void UBaseGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	// 정리 작업
	UnregisterFollowUpInputs();
	GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);
	StopAllMontages();

	// 상태 태그 정리
	SetAbilityPhase(FGameplayTag());
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedActorInfo = *ActorInfo;

	// 어빌리티 실행 전 현재 무기 상태 저장, 어빌리티로 무기전환
	if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
	{
		PreviousEquipmentState = Agent->GetInteractorState();
		if (HasAuthority(&ActivationInfo))
		{
			Agent->SwitchEquipment(EInteractorType::Ability);
		}
	}

	StartAbilityExecution();
}

void UBaseGameplayAbility::StartAbilityExecution()
{
	switch (ActivationType)
	{
	case EAbilityActivationType::Instant:
		// 즉시 실행
		SetAbilityPhase(FValorantGameplayTags::Get().State_Ability_Executing);
		HandleExecutePhase();
		break;

	case EAbilityActivationType::WithPrepare:
		// 준비 단계부터 시작
		SetAbilityPhase(FValorantGameplayTags::Get().State_Ability_Preparing);
		HandlePreparePhase();
		break;
	}
}

void UBaseGameplayAbility::HandlePreparePhase()
{
	// 하위 클래서에서 정의될 어빌리티 준비 로직
	PrepareAbility();
	
	// 준비 애니메이션 재생
	if (PrepareMontage_1P || PrepareMontage_3P)
	{
		PlayMontages(PrepareMontage_1P, PrepareMontage_3P);

		// 몽타주 완료 대기
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, PrepareMontage_3P, 1.0f);

		if (Task)
		{
			Task->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
			Task->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnMontageBlendOut);
			Task->OnInterrupted.AddDynamic(this, &UBaseGameplayAbility::OnMontageBlendOut);
			Task->OnCancelled.AddDynamic(this, &UBaseGameplayAbility::OnMontageBlendOut);
			Task->ReadyForActivation();
		}
		
		ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
		if (!Agent)
		{
			return;
		}
		
		// 1인칭 몽타주 재생
		if (PrepareMontage_1P)
		{
			Agent->Client_PlayFirstPersonMontage(PrepareMontage_1P);
		}
	}
	else
	{
		// 애니메이션이 없으면 바로 대기 단계로
		OnMontageCompleted();
	}
}

void UBaseGameplayAbility::HandleWaitingPhase()
{
	// 대기 애니메이션 재생 (루프)
	if (WaitingMontage_1P || WaitingMontage_3P)
	{
		PlayMontages(WaitingMontage_1P, WaitingMontage_3P, false);

		ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
		if (!Agent)
		{
			return;
		}
		
		// 1인칭 몽타주 재생
		if (WaitingMontage_1P)
		{
			Agent->Client_PlayFirstPersonMontage(WaitingMontage_1P);
		}
		// 1인칭 몽타주 재생
		if (WaitingMontage_3P)
		{
			Agent->NetMulti_PlayThirdPersonMontage(WaitingMontage_3P);
		}
	}

	// 하위 클래서에서 정의될 어빌리티 대기 로직
	WaitAbility();

	// 후속 입력 등록
	RegisterFollowUpInputs();

	// 타임아웃 설정 (default : 10초)
	GetWorld()->GetTimerManager().SetTimer(WaitingTimeoutHandle, this,
	                                       &UBaseGameplayAbility::OnWaitingTimeout, FollowUpTime, false);
}

void UBaseGameplayAbility::HandleExecutePhase()
{
	// 서버에서만 스택 소모 실행
	if (HasAuthority(&CurrentActivationInfo))
	{
		ReduceAbilityStack();
	}

	// 하위 클래서에서 정의될 어빌리티 실행 로직
	ExecuteAbility();
	
	// 대기 타이머 정리
	GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);

	// 실행 애니메이션 재생
	if (ExecuteMontage_1P || ExecuteMontage_3P)
	{
		ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
		if (!Agent)
		{
			return;
		}
		
		// 1인칭 몽타주 재생
		if (ExecuteMontage_1P)
		{
			Agent->Client_PlayFirstPersonMontage(ExecuteMontage_1P);
		}

		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ExecuteMontage_3P, 1.0f);

		if (Task)
		{
			Task->OnCompleted.AddDynamic(this, &UBaseGameplayAbility::OnMontageCompleted);
			Task->OnBlendOut.AddDynamic(this, &UBaseGameplayAbility::OnMontageBlendOut);
			Task->ReadyForActivation();
		}
	}
	else
	{
		// 애니메이션이 없으면 바로 종료
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UBaseGameplayAbility::PrepareAbility()
{
	FGameplayTagContainer OwnedTags = AbilityTags;
	
	const FGameplayTag skillTaglRoot = FGameplayTag::RequestGameplayTag(TEXT("Input.Skill"));

	FGameplayTag inputTag;
	for (const FGameplayTag& Tag : OwnedTags)
	{
		if (Tag.MatchesTag(skillTaglRoot))
		{
			inputTag = Tag;
			break;
		}
	}
	
	OnPrepareAbility.Broadcast(inputTag, FollowUpInputType);
}

void UBaseGameplayAbility::WaitAbility()
{
}

void UBaseGameplayAbility::ExecuteAbility()
{
}

bool UBaseGameplayAbility::OnLeftClickInput()
{
	return true;
}

bool UBaseGameplayAbility::OnRightClickInput()
{
	return true;
}

bool UBaseGameplayAbility::OnRepeatInput()
{
	return true;
}

void UBaseGameplayAbility::OnMontageCompleted()
{
	if (CurrentPhase == FValorantGameplayTags::Get().State_Ability_Preparing)
	{
		// 준비 완료 -> 대기 단계로
		SetAbilityPhase(FValorantGameplayTags::Get().State_Ability_Waiting);
		HandleWaitingPhase();
	}
	else if (CurrentPhase == FValorantGameplayTags::Get().State_Ability_Executing)
	{
		// 실행 완료 -> 어빌리티 종료
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UBaseGameplayAbility::OnMontageBlendOut()
{
	// 몽타주가 중단된 경우 처리
	if (IsActive())
	{
		OnMontageCompleted();
	}
}

void UBaseGameplayAbility::HandleFollowUpInput(FGameplayTag InputTag)
{
	if (CurrentPhase != FValorantGameplayTags::Get().State_Ability_Waiting)
	{
		return;
	}

	bool bShouldExecute = false;

	// 입력 타입에 따라 처리
	if (InputTag == FValorantGameplayTags::Get().InputTag_Default_LeftClick)
	{
		if (FollowUpInputType == EFollowUpInputType::LeftClick ||
			FollowUpInputType == EFollowUpInputType::LeftOrRight ||
			FollowUpInputType == EFollowUpInputType::Any)
		{
			bShouldExecute = OnLeftClickInput();
		}
	}
	else if (InputTag == FValorantGameplayTags::Get().InputTag_Default_RightClick)
	{
		if (FollowUpInputType == EFollowUpInputType::RightClick ||
			FollowUpInputType == EFollowUpInputType::LeftOrRight ||
			FollowUpInputType == EFollowUpInputType::Any)
		{
			bShouldExecute = OnRightClickInput();
		}
	}
	else if (InputTag == FValorantGameplayTags::Get().InputTag_Default_Repeat)
	{
		if (FollowUpInputType == EFollowUpInputType::RepeatKey ||
			FollowUpInputType == EFollowUpInputType::Any)
		{
			bShouldExecute = OnRepeatInput();
		}
	}

	if (bShouldExecute)
	{
		UnregisterFollowUpInputs();
		SetAbilityPhase(FValorantGameplayTags::Get().State_Ability_Executing);
		HandleExecutePhase();
	}
}

void UBaseGameplayAbility::PlayMontages(UAnimMontage* Montage1P, UAnimMontage* Montage3P, bool bStopAllMontages)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (bStopAllMontages)
	{
		StopAllMontages();
	}

	// 멀티 캐스트로 부를지 고려하기!
	ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
	if (!Agent)
	{
		return;
	}
	// 1인칭 몽타주
	if (Montage1P)
	{
		Agent->Client_PlayFirstPersonMontage(Montage3P);
	}
	

	// 1인칭 몽타주
	if (Montage1P)
	{
		Agent->Client_PlayFirstPersonMontage(Montage3P);
	}
}

void UBaseGameplayAbility::StopAllMontages()
{
	ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
	if (!Agent)
	{
		return;
	}

	// 3인칭 몽타주
	Agent->StopThirdPersonMontage(0.1f);

	// 1인칭 몽타주
	Agent->StopFirstPersonMontage(0.1f);
}

bool UBaseGameplayAbility::SpawnProjectile(FVector LocationOffset, FRotator RotationOffset)
{
	if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	// 카메라 위치에서 스폰
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f +
		LocationOffset;
	FRotator SpawnRotation = Character->GetControlRotation() + RotationOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedProjectile = GetWorld()->SpawnActor<ABaseProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	return true;
}

void UBaseGameplayAbility::SetAbilityPhase(FGameplayTag NewPhase)
{
	if (CurrentPhase == NewPhase)
	{
		return;
	}

	CurrentPhase = NewPhase;
	if (HasAuthority(&CurrentActivationInfo))
	{
		OnRep_CurrentPhase();
	}

	// 어빌리티 실행 단계일 때 무기 교체 제한 태그 추가
	if (NewPhase == FValorantGameplayTags::Get().State_Ability_Executing)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->AddLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
		}
	}
	// 다른 단계로 전환될 때 무기 사용 제한 태그 제거
	else
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
		}
	}
}

void UBaseGameplayAbility::OnRep_CurrentPhase()
{
	OnPhaseChanged.Broadcast(CurrentPhase);

	// ASC에 상태 태그 업데이트
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 모든 상태 태그 제거
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);

		// 현재 상태 태그 추가
		if (CurrentPhase.IsValid())
		{
			ASC->AddLooseGameplayTag(CurrentPhase);
		}
	}
}

void UBaseGameplayAbility::RegisterFollowUpInputs()
{
	if (FollowUpInputType == EFollowUpInputType::None)
	{
		return;
	}

	// AgentAbilitySystemComponent에 후속 입력 등록
	if (UAgentAbilitySystemComponent* ASC = Cast<
		UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		TSet<FGameplayTag> InputTags;

		switch (FollowUpInputType)
		{
		case EFollowUpInputType::LeftClick:
			InputTags.Add(FValorantGameplayTags::Get().InputTag_Default_LeftClick);
			break;
		case EFollowUpInputType::RightClick:
			InputTags.Add(FValorantGameplayTags::Get().InputTag_Default_RightClick);
			break;
		case EFollowUpInputType::RepeatKey:
			InputTags.Add(FValorantGameplayTags::Get().InputTag_Default_Repeat);
			break;
		case EFollowUpInputType::LeftOrRight:
			InputTags.Add(FValorantGameplayTags::Get().InputTag_Default_LeftClick);
			InputTags.Add(FValorantGameplayTags::Get().InputTag_Default_RightClick);
			break;
		case EFollowUpInputType::Any:
			InputTags.Add(FValorantGameplayTags::Get().InputTag_Default_LeftClick);
			InputTags.Add(FValorantGameplayTags::Get().InputTag_Default_RightClick);
			InputTags.Add(FValorantGameplayTags::Get().InputTag_Default_Repeat);
			break;
		}

		ASC->RegisterFollowUpInputs(InputTags, GetAssetTags().First());
	}
}

void UBaseGameplayAbility::UnregisterFollowUpInputs()
{
	if (UAgentAbilitySystemComponent* ASC = Cast<
		UAgentAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		ASC->ClearFollowUpInputs();
	}
}

void UBaseGameplayAbility::OnWaitingTimeout()
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility, bool bWasCancelled)
{
	// 정리
	UnregisterFollowUpInputs();
	GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);
	StopAllMontages();

	// 상태 태그 정리
	SetAbilityPhase(FGameplayTag());

	// 어빌리티 종료 시 원래 무기로 복귀
	if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
	{
		// 이전 무기 상태로 복귀
		if (PreviousEquipmentState != EInteractorType::None)
		{
			if (HasAuthority(&ActivationInfo))
			{
				Agent->SwitchEquipment(EInteractorType::Ability);
			}
			PreviousEquipmentState = EInteractorType::None;
		}
	}

	NET_LOG(LogTemp,Warning,TEXT("어빌리티 종료 브로드캐스트"));
	OnEndAbility.Broadcast();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UBaseGameplayAbility::ReduceAbilityStack()
{
	// PlayerState에서 차지 소모
	if (APlayerController* PC = Cast<APlayerController>(GetAvatarActorFromActorInfo()->GetInstigatorController()))
	{
		if (AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>())
		{
			return static_cast<bool>(PS->ReduceAbilityStack(m_AbilityID));
		}
	}
	return false;
}

int32 UBaseGameplayAbility::GetAbilityStack() const
{
	if (const APlayerController* PC = Cast<APlayerController>(GetAvatarActorFromActorInfo()->GetInstigatorController()))
	{
		if (const AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>())
		{
			return PS->GetAbilityStack(m_AbilityID);
		}
	}
	return 0;
}
