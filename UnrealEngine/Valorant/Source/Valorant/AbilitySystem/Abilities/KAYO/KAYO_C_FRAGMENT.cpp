#include "KAYO_C_FRAGMENT.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/KayO/KayoGrenade.h"
#include "AgentAbility/KayO/KayoGrenadeEquipped.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Agent/BaseAgent.h"

UKAYO_C_FRAGMENT::UKAYO_C_FRAGMENT(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	m_AbilityID = 3001;
	
	// FRAG/ment는 준비 후 클릭으로 던지기
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
	FollowUpTime = 15.0f; // 15초 대기 시간
}

void UKAYO_C_FRAGMENT::PrepareAbility()
{
	Super::PrepareAbility();
	
	// 장착된 수류탄 생성
	SpawnEquippedGrenades();
	
	UE_LOG(LogTemp, Warning, TEXT("KAYO C - FRAG/ment 준비 중"));
}

void UKAYO_C_FRAGMENT::WaitAbility()
{
	Super::WaitAbility();

	// 장착 애니메이션 실행
	if (SpawnedGrenade1P)
	{
		SpawnedGrenade1P->OnEquip();
	}
	if (SpawnedGrenade3P)
	{
		SpawnedGrenade3P->OnEquip();
	}
}

bool UKAYO_C_FRAGMENT::OnLeftClickInput()
{
	CurrentThrowType = EGrenadeThrowType::Overhand;
	return ThrowGrenade(CurrentThrowType);
}

bool UKAYO_C_FRAGMENT::OnRightClickInput()
{
	CurrentThrowType = EGrenadeThrowType::Underhand;
	return ThrowGrenade(CurrentThrowType);
}

void UKAYO_C_FRAGMENT::SpawnEquippedGrenades()
{
	if (!EquippedGrenadeClass)
		return;
	
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;
	
	// 서버에서 3인칭 수류탄 생성 (모든 클라이언트에 자동 복제)
	if (HasAuthority(&CurrentActivationInfo))
	{
		FVector HandLocation3P = OwnerAgent->GetMesh()->GetSocketLocation(FName("L_Hand"));
		FRotator HandRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams3P;
		SpawnParams3P.Owner = OwnerAgent;
		SpawnParams3P.Instigator = OwnerAgent;
		SpawnParams3P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedGrenade3P = GetWorld()->SpawnActor<AKayoGrenadeEquipped>(
			EquippedGrenadeClass, HandLocation3P, HandRotation, SpawnParams3P);
		
		if (SpawnedGrenade3P)
		{
			// 3인칭 수류탄으로 설정 (복제되도록)
			SpawnedGrenade3P->SetGrenadeViewType(EGrenadeViewType::ThirdPerson);
			
			// 3인칭 메쉬에 부착
			SpawnedGrenade3P->AttachToComponent(OwnerAgent->GetMesh(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("L_Hand"));
		}
	}
	
	// 로컬 플레이어인 경우에만 1인칭 수류탄 생성
	if (OwnerAgent->IsLocallyControlled())
	{
		FVector HandLocation1P = OwnerAgent->GetMesh1P()->GetSocketLocation(FName("L_WeaponPoint"));
		FRotator HandRotation = FRotator::ZeroRotator;
		
		FActorSpawnParameters SpawnParams1P;
		SpawnParams1P.Owner = OwnerAgent;
		SpawnParams1P.Instigator = OwnerAgent;
		SpawnParams1P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		SpawnedGrenade1P = GetWorld()->SpawnActor<AKayoGrenadeEquipped>(
			EquippedGrenadeClass, HandLocation1P, HandRotation, SpawnParams1P);
		
		if (SpawnedGrenade1P)
		{
			// 1인칭 수류탄으로 설정 (복제 안 됨)
			SpawnedGrenade1P->SetGrenadeViewType(EGrenadeViewType::FirstPerson);
			
			// 복제 비활성화
			SpawnedGrenade1P->SetReplicates(false);
			
			// 1인칭 메쉬에 부착
			SpawnedGrenade1P->AttachToComponent(OwnerAgent->GetMesh1P(), 
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("L_WeaponPoint"));
		}
	}
}

void UKAYO_C_FRAGMENT::DestroyEquippedGrenades()
{
	// 3인칭 수류탄 제거 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedGrenade3P)
	{
		SpawnedGrenade3P->OnUnequip();
		SpawnedGrenade3P->Destroy();
		SpawnedGrenade3P = nullptr;
	}
	
	// 1인칭 수류탄 제거 (로컬에서만)
	if (SpawnedGrenade1P)
	{
		SpawnedGrenade1P->OnUnequip();
		SpawnedGrenade1P->Destroy();
		SpawnedGrenade1P = nullptr;
	}
}

bool UKAYO_C_FRAGMENT::ThrowGrenade(EGrenadeThrowType ThrowType)
{
	// 던지기 타입 업데이트 (시각적 효과)
	bool bIsOverhand = (ThrowType == EGrenadeThrowType::Overhand);
	if (SpawnedGrenade3P)
	{
		SpawnedGrenade3P->SetThrowType(bIsOverhand);
	}
	if (SpawnedGrenade1P)
	{
		SpawnedGrenade1P->SetThrowType(bIsOverhand);
	}

	// 먼저 장착된 수류탄들 제거
	DestroyEquippedGrenades();

	// 투사체 생성
	bool bSuccess = SpawnProjectileByType(ThrowType);

	if (bSuccess)
	{
		// 던지기 사운드 재생
		if (ThrowSound)
		{
			ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
			if (Character)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ThrowSound, Character->GetActorLocation());
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("KAYO C - %s 수류탄 던지기 성공"), 
			bIsOverhand ? TEXT("오버핸드") : TEXT("언더핸드"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO C - 수류탄 던지기 실패"));
		return false;
	}
}

bool UKAYO_C_FRAGMENT::SpawnProjectileByType(EGrenadeThrowType ThrowType)
{
	if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO C - 권한 없음 또는 투사체 클래스 없음"));
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO C - 캐릭터 참조 실패"));
		return false;
	}

	// 수류탄 스폰 위치 계산
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f + FVector(0, 0, (ThrowType == EGrenadeThrowType::Underhand) ? 50.f : 150.0f);
	FRotator SpawnRotation = Character->GetControlRotation();

	// 언더핸드의 경우 각도 조정 (아래쪽으로)
	if (ThrowType == EGrenadeThrowType::Underhand)
	{
		SpawnRotation.Pitch = FMath::Max(SpawnRotation.Pitch - 15.0f, -45.0f);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AKayoGrenade* Grenade = GetWorld()->SpawnActor<AKayoGrenade>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (Grenade)
	{
		// 던지기 타입 설정
		EKayoGrenadeThrowType GrenadeThrowType = (ThrowType == EGrenadeThrowType::Underhand) ? 
			EKayoGrenadeThrowType::Underhand : EKayoGrenadeThrowType::Overhand;
		Grenade->SetThrowType(GrenadeThrowType);
		
		SpawnedProjectile = Grenade;
		
		// 발사 효과 재생
		PlayCommonEffects(ProjectileLaunchEffect, ProjectileLaunchSound, SpawnLocation);
		
		UE_LOG(LogTemp, Warning, TEXT("KAYO C - %s 수류탄 생성 및 설정 완료"), 
			(ThrowType == EGrenadeThrowType::Underhand) ? TEXT("언더핸드") : TEXT("오버핸드"));
		
		return true;
	}
	
	return false;
}

void UKAYO_C_FRAGMENT::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 장착된 수류탄들 정리
	DestroyEquippedGrenades();
	
	// 상태 초기화
	CurrentThrowType = EGrenadeThrowType::None;
	
	// 부모 클래스 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}