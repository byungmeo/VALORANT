#include "KAYO_E_ZEROPOINT.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/KayO/KayoKnife.h"
#include "AgentAbility/KayO/KayoKnifeEquipped.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Agent/BaseAgent.h"
#include "TimerManager.h"

UKAYO_E_ZEROPOINT::UKAYO_E_ZEROPOINT(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	m_AbilityID = 3003;
	
	// ZERO/point는 준비 후 클릭으로 던지기
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftClick;
}

void UKAYO_E_ZEROPOINT::PrepareAbility()
{
	Super::PrepareAbility();
	
	// 장착된 나이프 생성
	SpawnEquippedKnives();
	
	UE_LOG(LogTemp, Warning, TEXT("KAYO E - ZERO/point 준비 중"));
}

void UKAYO_E_ZEROPOINT::WaitAbility()
{
	Super::WaitAbility();

	// 장착 애니메이션 실행
	if (SpawnedKnife1P)
	{
		SpawnedKnife1P->OnEquip();
	}
}

bool UKAYO_E_ZEROPOINT::OnLeftClickInput()
{
	// 먼저 장착된 나이프들 제거
	DestroyEquippedKnives();
	
	// 좌클릭으로 나이프 던지기
	return ThrowKnife();
}

void UKAYO_E_ZEROPOINT::SpawnEquippedKnives()
{
	if (!EquippedKnifeClass)
		return;
	
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;
	
	// 서버에서 3인칭 나이프 생성 (모든 클라이언트에 자동 복제)
	if (HasAuthority(&CurrentActivationInfo))
	{
		FVector HandLocation3P = OwnerAgent->GetMesh()->GetSocketLocation(FName("R_Hand"));
		FRotator HandRotation = FRotator::ZeroRotator;

		
		FActorSpawnParameters SpawnParams3P;
		SpawnParams3P.Owner = OwnerAgent;
		SpawnParams3P.Instigator = OwnerAgent;
		SpawnParams3P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedKnife3P = GetWorld()->SpawnActor<AKayoKnifeEquipped>(
			EquippedKnifeClass, HandLocation3P, HandRotation, SpawnParams3P);
		
		if (SpawnedKnife3P)
		{
			// 3인칭 나이프로 설정 (복제되도록)
			SpawnedKnife3P->SetKnifeViewType(EKnifeViewType::ThirdPerson);
			
			// 3인칭 메쉬에 부착
			SpawnedKnife3P->AttachToComponent(OwnerAgent->GetMesh(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_Hand"));
				
			// 장착 애니메이션 실행
			SpawnedKnife3P->OnEquip();
		}
	}
	
	// 로컬 플레이어인 경우에만 1인칭 나이프 생성
	if (OwnerAgent->IsLocallyControlled())
	{
		FVector HandLocation1P = OwnerAgent->GetMesh1P()->GetSocketLocation(FName("R_WeaponPoint"));
		FRotator HandRotation = FRotator::ZeroRotator;
		
		FActorSpawnParameters SpawnParams1P;
		SpawnParams1P.Owner = OwnerAgent;
		SpawnParams1P.Instigator = OwnerAgent;
		SpawnParams1P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedKnife1P = GetWorld()->SpawnActor<AKayoKnifeEquipped>(
			EquippedKnifeClass, HandLocation1P, HandRotation, SpawnParams1P);
		
		if (SpawnedKnife1P)
		{
			// 1인칭 나이프로 설정 (복제 안 됨)
			SpawnedKnife1P->SetKnifeViewType(EKnifeViewType::FirstPerson);
			
			// 복제 비활성화
			SpawnedKnife1P->SetReplicates(false);
			
			// 1인칭 메쉬에 부착
			SpawnedKnife1P->AttachToComponent(OwnerAgent->GetMesh1P(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_WeaponPoint"));
		}
	}
}

void UKAYO_E_ZEROPOINT::DestroyEquippedKnives()
{
	// 3인칭 나이프 제거 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedKnife3P)
	{
		SpawnedKnife3P->OnUnequip();
		SpawnedKnife3P->Destroy();
		SpawnedKnife3P = nullptr;
	}
	
	// 1인칭 나이프 제거 (로컬에서만)
	if (SpawnedKnife1P)
	{
		SpawnedKnife1P->OnUnequip();
		SpawnedKnife1P->Destroy();
		SpawnedKnife1P = nullptr;
	}
	
	// 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(KnifeUpdateTimer);
	}
}

void UKAYO_E_ZEROPOINT::UpdateKnifePositions()
{
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;
	
	// 3인칭 나이프 위치 업데이트 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedKnife3P)
	{
		// 3인칭 나이프는 이미 부착되어 있으므로 추가 업데이트 불필요
		// 필요시 추가 로직 구현
	}
	
	// 1인칭 나이프 위치 업데이트 (로컬에서만)
	if (SpawnedKnife1P && OwnerAgent->IsLocallyControlled())
	{
		// 1인칭 나이프는 이미 부착되어 있으므로 추가 업데이트 불필요
		// 필요시 추가 로직 구현
	}
}

bool UKAYO_E_ZEROPOINT::ThrowKnife()
{
	if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO E - 권한 없음 또는 투사체 클래스 없음"));
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (nullptr == Character)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO E - 캐릭터 참조 실패"));
		return false;
	}

	// 실제 투사체 나이프 스폰 위치 계산
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f + FVector(0, 0, 50.0f);
	FRotator SpawnRotation = Character->GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 실제 투사체 나이프 생성 (기존 KayoKnife 클래스 사용)
	AKayoKnife* Knife = GetWorld()->SpawnActor<AKayoKnife>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (Knife)
	{
		// 나이프 던지기 활성화
		Knife->OnThrow();
		SpawnedProjectile = Knife;
		
		// 발사 효과 재생
		PlayCommonEffects(ProjectileLaunchEffect, ProjectileLaunchSound, SpawnLocation);
		
		UE_LOG(LogTemp, Warning, TEXT("KAYO E - 억제 나이프 생성 및 던지기 성공"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO E - 억제 나이프 생성 실패"));
		return false;
	}
}

void UKAYO_E_ZEROPOINT::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 장착된 나이프들 정리
	DestroyEquippedKnives();
	
	// 부모 클래스 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}