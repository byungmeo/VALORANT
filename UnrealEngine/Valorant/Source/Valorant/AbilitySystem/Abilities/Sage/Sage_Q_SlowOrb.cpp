#include "Sage_Q_SlowOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/Sage/SlowOrb.h"
#include "AgentAbility/Sage/SageSlowOrbEquipped.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Agent/BaseAgent.h"
#include "GameFramework/Character.h"

USage_Q_SlowOrb::USage_Q_SlowOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	m_AbilityID = 1002;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
	FollowUpTime = 8.0f; // 8초 대기 시간
}

void USage_Q_SlowOrb::PrepareAbility()
{
	Super::PrepareAbility();
	
	// 준비 효과 재생
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		PlayCommonEffects(PrepareEffect, PrepareSound, Character->GetActorLocation());
	}
	
	// 장착된 슬로우 오브 생성
	SpawnEquippedSlowOrbs();
	
	UE_LOG(LogTemp, Warning, TEXT("Sage Q - Slow Orb 준비 중"));
}

void USage_Q_SlowOrb::WaitAbility()
{
	Super::WaitAbility();
	
	// 장착 애니메이션 실행
	if (SpawnedSlowOrb1P)
	{
		SpawnedSlowOrb1P->OnEquip();
	}
	if (SpawnedSlowOrb3P)
	{
		SpawnedSlowOrb3P->OnEquip();
	}
	
	// 준비 사운드 재생 (로컬만)
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (OwnerAgent && OwnerAgent->IsLocallyControlled())
	{
		if (GetWorld() && PrepareSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), PrepareSound);
		}
	}
}

bool USage_Q_SlowOrb::OnLeftClickInput()
{
	// 슬로우 오브 던지기
	return ThrowSlowOrb();
}

bool USage_Q_SlowOrb::OnRightClickInput()
{
	// 세이지 Q는 우클릭도 동일하게 던지기 (언더핸드 없음)
	return ThrowSlowOrb();
}

void USage_Q_SlowOrb::SpawnEquippedSlowOrbs()
{
	if (!EquippedSlowOrbClass)
		return;
	
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;
	
	// 서버에서 3인칭 슬로우 오브 생성 (모든 클라이언트에 자동 복제)
	if (HasAuthority(&CurrentActivationInfo))
	{
		FVector HandLocation3P = OwnerAgent->GetMesh()->GetSocketLocation(FName("R_WeaponPoint"));
		FRotator HandRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams3P;
		SpawnParams3P.Owner = OwnerAgent;
		SpawnParams3P.Instigator = OwnerAgent;
		SpawnParams3P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedSlowOrb3P = GetWorld()->SpawnActor<ASageSlowOrbEquipped>(
			EquippedSlowOrbClass, HandLocation3P, HandRotation, SpawnParams3P);
		
		if (SpawnedSlowOrb3P)
		{
			// 3인칭 슬로우 오브로 설정 (복제되도록)
			SpawnedSlowOrb3P->SetSlowOrbViewType(EViewType::ThirdPerson);
			
			// 3인칭 메쉬에 부착
			SpawnedSlowOrb3P->AttachToComponent(OwnerAgent->GetMesh(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_WeaponPoint"));
		}
	}
	
	// 로컬 플레이어인 경우에만 1인칭 슬로우 오브 생성
	if (OwnerAgent->IsLocallyControlled())
	{
		FVector HandLocation1P = OwnerAgent->GetMesh1P()->GetSocketLocation(FName("R_WeaponPoint"));
		FRotator HandRotation = FRotator::ZeroRotator;
		
		FActorSpawnParameters SpawnParams1P;
		SpawnParams1P.Owner = OwnerAgent;
		SpawnParams1P.Instigator = OwnerAgent;
		SpawnParams1P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedSlowOrb1P = GetWorld()->SpawnActor<ASageSlowOrbEquipped>(
			EquippedSlowOrbClass, HandLocation1P, HandRotation, SpawnParams1P);
		
		if (SpawnedSlowOrb1P)
		{
			// 1인칭 슬로우 오브로 설정 (복제 안 됨)
			SpawnedSlowOrb1P->SetSlowOrbViewType(EViewType::FirstPerson);
			
			// 복제 비활성화
			SpawnedSlowOrb1P->SetReplicates(false);
			
			// 1인칭 메쉬에 부착
			SpawnedSlowOrb1P->AttachToComponent(OwnerAgent->GetMesh1P(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_WeaponPoint"));
		}
	}
}

void USage_Q_SlowOrb::DestroyEquippedSlowOrbs()
{
	// 3인칭 슬로우 오브 제거 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedSlowOrb3P)
	{
		SpawnedSlowOrb3P->OnUnequip();
		SpawnedSlowOrb3P->Destroy();
		SpawnedSlowOrb3P = nullptr;
	}
	
	// 1인칭 슬로우 오브 제거 (로컬에서만)
	if (SpawnedSlowOrb1P)
	{
		SpawnedSlowOrb1P->OnUnequip();
		SpawnedSlowOrb1P->Destroy();
		SpawnedSlowOrb1P = nullptr;
	}
}

bool USage_Q_SlowOrb::ThrowSlowOrb()
{
	// 먼저 장착된 슬로우 오브들 제거
	DestroyEquippedSlowOrbs();
	
	// 기존 SpawnProjectile 사용
	bool bSuccess = SpawnProjectile();
	
	if (bSuccess)
	{
		// 던지기 사운드 재생
		if (ThrowSound)
		{
			if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ThrowSound, Character->GetActorLocation());
			}
		}
		
		// 실행 사운드 재생
		if (ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get()))
		{
			if (ExecuteSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExecuteSound, OwnerAgent->GetActorLocation());
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Sage Q - Slow Orb 던지기 성공"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Sage Q - Slow Orb 던지기 실패"));
		return false;
	}
}

void USage_Q_SlowOrb::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 장착된 슬로우 오브들 정리
	DestroyEquippedSlowOrbs();
	
	// 부모 클래스 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}