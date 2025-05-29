#include "KAYO_C_FRAGMENT.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/KayO/KayoGrenade.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UKAYO_C_FRAGMENT::UKAYO_C_FRAGMENT(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	m_AbilityID = 3001;
	
	// FRAG/ment는 즉시 실행 타입 (던지기)
	ActivationType = EAbilityActivationType::Instant;
}

void UKAYO_C_FRAGMENT::ExecuteAbility()
{
	if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO C - 권한 없음 또는 투사체 클래스 없음"));
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO C - 캐릭터 참조 실패"));
		return;
	}

	// 수류탄 스폰 위치 계산
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f + FVector(0, 0, 50.0f);
	FRotator SpawnRotation = Character->GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AKayoGrenade* Grenade = GetWorld()->SpawnActor<AKayoGrenade>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (Grenade)
	{
		SpawnedProjectile = Grenade;
		
		// 발사 효과 재생
		PlayCommonEffects(ProjectileLaunchEffect, ProjectileLaunchSound, SpawnLocation);
		
		// 실행 효과 재생
		PlayCommonEffects(ExecuteEffect, ExecuteSound, Character->GetActorLocation());
		
		UE_LOG(LogTemp, Warning, TEXT("KAYO C - FRAG/ment 투사체 생성 성공"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO C - FRAG/ment 투사체 생성 실패"));
	}
}