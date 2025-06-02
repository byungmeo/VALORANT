#include "KAYO_Q_FLASHDRIVE.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/KayO/Flashbang.h"
#include "AgentAbility/KayO/KayoFlashbangEquipped.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Agent/BaseAgent.h"

UKAYO_Q_FLASHDRIVE::UKAYO_Q_FLASHDRIVE(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	m_AbilityID = 3002;
	
	// FLASH/drive는 준비 후 좌클릭/우클릭으로 던지기 방식 선택
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
	FollowUpTime = 15.0f; // 15초 대기 시간
}

void UKAYO_Q_FLASHDRIVE::PrepareAbility()
{
	Super::PrepareAbility();
	
	// 장착된 플래시뱅 생성
	SpawnEquippedFlashbangs();
	
	UE_LOG(LogTemp, Warning, TEXT("KAYO Q - FLASH/drive 준비 중"));
}

void UKAYO_Q_FLASHDRIVE::WaitAbility()
{
	Super::WaitAbility();

	// 장착 애니메이션 실행
	if (SpawnedFlashbang1P)
	{
		SpawnedFlashbang1P->OnEquip();
	}
	if (SpawnedFlashbang3P)
	{
		SpawnedFlashbang3P->OnEquip();
	}
}

bool UKAYO_Q_FLASHDRIVE::OnLeftClickInput()
{
	// 좌클릭: 직선 던지기 미리보기
	if (SpawnedFlashbang1P)
	{
		SpawnedFlashbang1P->SetThrowPreview(false);
	}
	if (SpawnedFlashbang3P)
	{
		SpawnedFlashbang3P->SetThrowPreview(false);
	}
	
	// 플래시뱅 던지기
	return ThrowFlashbang(false);
}

bool UKAYO_Q_FLASHDRIVE::OnRightClickInput()
{
	// 우클릭: 포물선 던지기 미리보기
	if (SpawnedFlashbang1P)
	{
		SpawnedFlashbang1P->SetThrowPreview(true);
	}
	if (SpawnedFlashbang3P)
	{
		SpawnedFlashbang3P->SetThrowPreview(true);
	}
	
	// 플래시뱅 던지기
	return ThrowFlashbang(true);
}

void UKAYO_Q_FLASHDRIVE::SpawnEquippedFlashbangs()
{
	if (!EquippedFlashbangClass)
		return;
	
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;
	
	// 서버에서 3인칭 플래시뱅 생성 (모든 클라이언트에 자동 복제)
	if (HasAuthority(&CurrentActivationInfo))
	{
		FVector HandLocation3P = OwnerAgent->GetMesh()->GetSocketLocation(FName("R_Hand"));
		FRotator HandRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams3P;
		SpawnParams3P.Owner = OwnerAgent;
		SpawnParams3P.Instigator = OwnerAgent;
		SpawnParams3P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedFlashbang3P = GetWorld()->SpawnActor<AKayoFlashbangEquipped>(
			EquippedFlashbangClass, HandLocation3P, HandRotation, SpawnParams3P);
		
		if (SpawnedFlashbang3P)
		{
			// 3인칭 플래시뱅으로 설정 (복제되도록)
			SpawnedFlashbang3P->SetFlashbangViewType(EFlashbangViewType::ThirdPerson);
			
			// 3인칭 메쉬에 부착
			SpawnedFlashbang3P->AttachToComponent(OwnerAgent->GetMesh(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_Hand"));
		}
	}
	
	// 로컬 플레이어인 경우에만 1인칭 플래시뱅 생성
	if (OwnerAgent->IsLocallyControlled())
	{
		FVector HandLocation1P = OwnerAgent->GetMesh1P()->GetSocketLocation(FName("R_WeaponPoint"));
		FRotator HandRotation = FRotator::ZeroRotator;
		
		FActorSpawnParameters SpawnParams1P;
		SpawnParams1P.Owner = OwnerAgent;
		SpawnParams1P.Instigator = OwnerAgent;
		SpawnParams1P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedFlashbang1P = GetWorld()->SpawnActor<AKayoFlashbangEquipped>(
			EquippedFlashbangClass, HandLocation1P, HandRotation, SpawnParams1P);
		
		if (SpawnedFlashbang1P)
		{
			// 1인칭 플래시뱅으로 설정 (복제 안 됨)
			SpawnedFlashbang1P->SetFlashbangViewType(EFlashbangViewType::FirstPerson);
			
			// 복제 비활성화
			SpawnedFlashbang1P->SetReplicates(false);
			
			// 1인칭 메쉬에 부착
			SpawnedFlashbang1P->AttachToComponent(OwnerAgent->GetMesh1P(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("R_WeaponPoint"));
		}
	}
}

void UKAYO_Q_FLASHDRIVE::DestroyEquippedFlashbangs()
{
	// 3인칭 플래시뱅 제거 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedFlashbang3P)
	{
		SpawnedFlashbang3P->OnUnequip();
		SpawnedFlashbang3P->Destroy();
		SpawnedFlashbang3P = nullptr;
	}
	
	// 1인칭 플래시뱅 제거 (로컬에서만)
	if (SpawnedFlashbang1P)
	{
		SpawnedFlashbang1P->OnUnequip();
		SpawnedFlashbang1P->Destroy();
		SpawnedFlashbang1P = nullptr;
	}
}

bool UKAYO_Q_FLASHDRIVE::ThrowFlashbang(bool bAltFire)
{
	// 먼저 장착된 플래시뱅들 제거
	DestroyEquippedFlashbangs();

	if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO Q - 권한 없음 또는 투사체 클래스 없음"));
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO Q - 캐릭터 참조 실패"));
		return false;
	}

	// 플래시뱅 스폰 위치 계산
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f + FVector(0, 0, bAltFire ? 50.0f : 150.0f);
	FRotator SpawnRotation = Character->GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFlashbang* Flashbang = GetWorld()->SpawnActor<AFlashbang>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (Flashbang)
	{
		// 플래시뱅의 ActiveProjectileMovement 함수 호출 (bAltFire 파라미터로 던지기 방식 결정)
		Flashbang->ActiveProjectileMovement(bAltFire);
		SpawnedProjectile = Flashbang;
		
		// 던지기 사운드 재생
		if (ThrowSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ThrowSound, Character->GetActorLocation());
		}
		
		// 발사 효과 재생
		PlayCommonEffects(ProjectileLaunchEffect, ProjectileLaunchSound, SpawnLocation);
		
		UE_LOG(LogTemp, Warning, TEXT("KAYO Q - 플래시뱅 생성 성공 (AltFire: %s)"), 
			bAltFire ? TEXT("true - 포물선") : TEXT("false - 직선"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO Q - 플래시뱅 생성 실패"));
		return false;
	}
}

void UKAYO_Q_FLASHDRIVE::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 장착된 플래시뱅들 정리
	DestroyEquippedFlashbangs();
	
	// 부모 클래스 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}