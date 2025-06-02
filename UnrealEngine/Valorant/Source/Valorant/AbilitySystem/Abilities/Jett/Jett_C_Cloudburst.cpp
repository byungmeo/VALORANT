#include "Jett_C_Cloudburst.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "GameFramework/Character.h"

UJett_C_Cloudburst::UJett_C_Cloudburst(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	
	m_AbilityID = 4001;
	ActivationType = EAbilityActivationType::Instant;
}

void UJett_C_Cloudburst::ExecuteAbility()
{
	ACharacter* Character = Cast<ACharacter>(CachedActorInfo.AvatarActor.Get());
	if (Character)
	{
		PlayCommonEffects(ExecuteEffect, ExecuteSound, Character->GetActorLocation());
	}
	
	SpawnProjectile();
}
