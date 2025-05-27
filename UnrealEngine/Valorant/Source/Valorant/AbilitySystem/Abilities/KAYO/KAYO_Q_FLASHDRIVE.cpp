#include "KAYO_Q_FLASHDRIVE.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/KayO/Flashbang.h"
#include "GameFramework/Character.h"

UKAYO_Q_FLASHDRIVE::UKAYO_Q_FLASHDRIVE(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);
	m_AbilityID = 3002;
	
	// FLASH/drive는 준비 후 좌클릭/우클릭으로 던지기 방식 선택
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
}

bool UKAYO_Q_FLASHDRIVE::OnLeftClickInput()
{
	// 좌클릭: 직선 던지기 (빠른 속도)
	return ThrowFlashbang(false);
}

bool UKAYO_Q_FLASHDRIVE::OnRightClickInput()
{
	// 우클릭: 포물선 던지기 (느린 속도, 높은 궤도)
	return ThrowFlashbang(true);
}

bool UKAYO_Q_FLASHDRIVE::ThrowFlashbang(bool bAltFire)
{
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

	// 플래시뱅 스폰
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f;
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
		
		UE_LOG(LogTemp, Warning, TEXT("KAYO Q - 플래시뱅 생성 성공 (AltFire: %s)"), 
			bAltFire ? TEXT("true") : TEXT("false"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO Q - 플래시뱅 생성 실패"));
		return false;
	}
}