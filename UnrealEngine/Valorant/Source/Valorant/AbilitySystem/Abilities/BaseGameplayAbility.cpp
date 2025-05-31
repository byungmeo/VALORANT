#include "BaseGameplayAbility.h"

#include <GameManager/SubsystemSteamManager.h>

#include "Valorant.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AgentAbility/BaseProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AgentPlayerState.h"
#include "Player/Agent/BaseAgent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Sound/SoundBase.h"
#include "NiagaraFunctionLibrary.h"

UBaseGameplayAbility::UBaseGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bReplicateInputDirectly = true;

	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Suppressed")));
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

void UBaseGameplayAbility::TransitionToState(EAbilityState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	// 이전 상태 퇴출
	switch (CurrentState)
	{
	case EAbilityState::Preparing:
		ExitState_Preparing();
		break;
	case EAbilityState::Waiting:
		ExitState_Waiting();
		break;
	case EAbilityState::Executing:
		ExitState_Executing();
		break;
	}

	// 상태 변경
	EAbilityState OldState = CurrentState;
	CurrentState = NewState;

	// 새 상태 진입
	switch (NewState)
	{
	case EAbilityState::Preparing:
		EnterState_Preparing();
		break;
	case EAbilityState::Waiting:
		EnterState_Waiting();
		break;
	case EAbilityState::Executing:
		EnterState_Executing();
		break;
	case EAbilityState::Canceling:
		EnterState_Canceling();
		break;
	case EAbilityState::Ending:
		EnterState_Ending();
		break;
	}

	// 델리게이트 브로드캐스트
	OnStateChanged.Broadcast(ConvertAbilityStateToGamePlayTag(NewState));

	// 디버그 로그
	NET_LOG(LogTemp, Display, TEXT("어빌리티 상태 전환: %s -> %s"),
	        *UEnum::GetValueAsString(OldState),
	        *UEnum::GetValueAsString(NewState));
}

void UBaseGameplayAbility::EnterState_Preparing()
{
	// ASC에 상태 태그 추가
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
		ASC->AddLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
	}

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

		// 1인칭 몽타주 재생
		if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
		{
			if (PrepareMontage_1P)
			{
				Agent->Client_PlayFirstPersonMontage(PrepareMontage_1P);
			}
		}
	}
	else
	{
		// 애니메이션이 없으면 바로 대기 단계로
		TransitionToState(EAbilityState::Waiting);
	}
}

void UBaseGameplayAbility::ExitState_Preparing()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
	}
}

void UBaseGameplayAbility::EnterState_Waiting()
{
	// ASC에 상태 태그 추가
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
	}

	// 대기 애니메이션 재생 (루프)
	if (WaitingMontage_1P || WaitingMontage_3P)
	{
		PlayMontages(WaitingMontage_1P, WaitingMontage_3P, false);
	}

	WaitAbility();

	// 후속 입력 등록
	RegisterFollowUpInputs();

	// 타임아웃 설정
	GetWorld()->GetTimerManager().SetTimer(WaitingTimeoutHandle, this,
	                                       &UBaseGameplayAbility::OnWaitingTimeout, FollowUpTime, false);
}

void UBaseGameplayAbility::ExitState_Waiting()
{
	// 타이머 정리
	GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);

	// 후속 입력 해제
	UnregisterFollowUpInputs();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
	}
}

void UBaseGameplayAbility::EnterState_Executing()
{
	// ASC에 상태 태그 추가
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
	}

	// 서버에서만 스택 소모
	if (HasAuthority(&CurrentActivationInfo))
	{
		ReduceAbilityStack();
	}

	ExecuteAbility();

	// 실행 애니메이션 재생
	if (ExecuteMontage_1P || ExecuteMontage_3P)
	{
		if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
		{
			if (ExecuteMontage_1P)
			{
				Agent->Client_PlayFirstPersonMontage(ExecuteMontage_1P);
			}
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
		TransitionToState(EAbilityState::Ending);
	}
}

void UBaseGameplayAbility::ExitState_Executing()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
	}
}

void UBaseGameplayAbility::EnterState_Canceling()
{
	CleanupAbility();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UBaseGameplayAbility::EnterState_Ending()
{
	CleanupAbility();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBaseGameplayAbility::CleanupAbility()
{
	// 모든 정리 작업을 한 곳에서 처리

	CurrentState = EAbilityState::None;

	// 타이머 정리
	GetWorld()->GetTimerManager().ClearTimer(WaitingTimeoutHandle);

	// 후속 입력 정리
	UnregisterFollowUpInputs();

	// 몽타주 정지
	StopAllMontages();

	// 모든 어빌리티 관련 태그 제거
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
		ASC->RemoveLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
	}

	// 무기 복귀 처리
	if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
	{
		if (PreviousEquipmentState != EInteractorType::None &&
			Agent->GetInteractorState() == EInteractorType::Ability)
		{
			// 약간의 딜레이 후 무기 전환
			FTimerHandle WeaponSwapTimer;
			GetWorld()->GetTimerManager().SetTimer(WeaponSwapTimer, [Agent, this]()
			{
				Agent->SwitchEquipment(PreviousEquipmentState);
			}, 0.1f, false);
		}
	}

	OnEndAbility.Broadcast();
}


bool UBaseGameplayAbility::CanBeCanceled() const
{
	// 실행 단계에서는 설정에 따라 취소 가능 여부 결정
	if (CurrentState == EAbilityState::Executing)
	{
		return bAllowCancelDuringExecution;
	}

	// 준비나 대기 단계에서는 항상 취소 가능
	return CurrentState == EAbilityState::Preparing || CurrentState == EAbilityState::Waiting;
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
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                         bool bReplicateCancelAbility)
{
	// 이미 취소/종료 중이면 무시
	if (CurrentState == EAbilityState::Canceling || CurrentState == EAbilityState::Ending)
	{
		return;
	}

	TransitionToState(EAbilityState::Canceling);
}

void UBaseGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      bool bReplicateEndAbility, bool bWasCancelled)
{
	// 이미 종료 중이면 무시
	if (CurrentState == EAbilityState::Ending || CurrentState == EAbilityState::Canceling)
	{
		return;
	}

	CurrentState = EAbilityState::None;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBaseGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedActorInfo = *ActorInfo;

	// 현재 무기 상태 저장
	if (ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo()))
	{
		PreviousEquipmentState = Agent->GetInteractorState();
		if (HasAuthority(&ActivationInfo))
		{
			Agent->SwitchEquipment(EInteractorType::Ability);
		}
	}

	// 어빌리티 타입에 따라 시작 상태 결정
	switch (ActivationType)
	{
	case EAbilityActivationType::Instant:
		TransitionToState(EAbilityState::Executing);
		break;

	case EAbilityActivationType::WithPrepare:
		TransitionToState(EAbilityState::Preparing);
		break;
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
	return false;
}

bool UBaseGameplayAbility::OnRightClickInput()
{
	return false;
}

bool UBaseGameplayAbility::OnRepeatInput()
{
	return false;
}

void UBaseGameplayAbility::OnMontageCompleted()
{
	switch (CurrentState)
	{
	case EAbilityState::Preparing:
		// 준비 완료 -> 대기 단계로
		TransitionToState(EAbilityState::Waiting);
		break;

	case EAbilityState::Executing:
		// 실행 완료 -> 어빌리티 종료
		TransitionToState(EAbilityState::Ending);
		break;
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
	if (CurrentState != EAbilityState::Waiting)
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
		TransitionToState(EAbilityState::Executing);
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

void UBaseGameplayAbility::StopAllMontages() const
{
	ABaseAgent* Agent = Cast<ABaseAgent>(GetAvatarActorFromActorInfo());
	if (!Agent)
	{
		return;
	}

	// 죽음 몽타주 재생 끊기 방지
	if (Agent->IsDead())
	{
		return;
	}

	// 3인칭 몽타주
	Agent->StopThirdPersonMontage(0.05f);

	// 1인칭 몽타주
	Agent->StopFirstPersonMontage(0.05f);
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

	// 카메라 컴포넌트 가져오기
	UCameraComponent* CameraComp = Character->FindComponentByClass<UCameraComponent>();
	FVector SpawnLocation;
	FRotator SpawnRotation;

	if (CameraComp)
	{
		// 카메라 위치에서 약간 앞쪽/아래쪽에서 스폰 (손 위치처럼)
		SpawnLocation = CameraComp->GetComponentLocation() +
			CameraComp->GetForwardVector() * 30.0f +
			CameraComp->GetRightVector() * 15.0f + // 오른손 위치
			CameraComp->GetUpVector() * -10.0f + // 약간 아래
			LocationOffset;

		SpawnRotation = CameraComp->GetComponentRotation() + RotationOffset;
	}
	else
	{
		// 폴백: 캐릭터 눈 높이에서 스폰
		FVector EyeLocation = Character->GetActorLocation() + FVector(0, 0, Character->BaseEyeHeight);
		SpawnLocation = EyeLocation + Character->GetActorForwardVector() * 50.0f + LocationOffset;
		SpawnRotation = Character->GetControlRotation() + RotationOffset;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedProjectile = GetWorld()->SpawnActor<ABaseProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	return SpawnedProjectile != nullptr;
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

bool UBaseGameplayAbility::ReduceAbilityStack()
{
	// PlayerState에서 차지 소모
	// if (const APlayerController* PC = Cast<APlayerController>(GetAvatarActorFromActorInfo()->GetInstigatorController()))
	if (const APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get()))
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
	// if (const APlayerController* PC = Cast<APlayerController>(GetAvatarActorFromActorInfo()->GetInstigatorController()))
	if (const APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get()))
	{
		if (const AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>())
		{
			return PS->GetAbilityStack(m_AbilityID);
		}
	}
	return 0;
}

void UBaseGameplayAbility::PlayCommonEffects(UNiagaraSystem* NiagaraEffect, USoundBase* SoundEffect, FVector Location)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	// 위치가 제공되지 않으면 캐릭터 위치 사용
	if (Location.IsZero())
	{
		Location = Character->GetActorLocation();
	}

	// 나이아가라 이펙트 재생
	if (NiagaraEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NiagaraEffect, Location);
	}

	// 사운드 효과 재생
	if (SoundEffect)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundEffect, Location);
	}
}

FGameplayTag UBaseGameplayAbility::ConvertAbilityStateToGamePlayTag(EAbilityState NewState)
{
	switch (NewState)
	{
	case EAbilityState::Preparing:
		{
			return FValorantGameplayTags::Get().State_Ability_Preparing;
		}
	case EAbilityState::Waiting:
		{
			return FValorantGameplayTags::Get().State_Ability_Waiting;
		}
	case EAbilityState::Executing:
		{
			return FValorantGameplayTags::Get().State_Ability_Executing;
		}
	case EAbilityState::Canceling:
		{
			return FValorantGameplayTags::Get().State_Ability_Canceling;
		}
	case EAbilityState::Ending:
		{
			return FValorantGameplayTags::Get().State_Ability_Ending;
		}
	default:
		{
			return FGameplayTag();
		}
	}
}
