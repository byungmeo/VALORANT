#include "KAYO_C_FRAGMENT.h"
#include "AbilitySystem/ValorantGameplayTags.h"

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
	// KayoGrenade 투사체 생성
	if (SpawnProjectile())
	{
		UE_LOG(LogTemp, Warning, TEXT("KAYO C - FRAG/ment 투사체 생성 성공"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO C - FRAG/ment 투사체 생성 실패"));
	}
}