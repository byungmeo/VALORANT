#include "Sage_Q_SlowOrb.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Agent/BaseAgent.h"

USage_Q_SlowOrb::USage_Q_SlowOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 1002;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
}

void USage_Q_SlowOrb::WaitAbility()
{
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
	SpawnProjectile();

	if (ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get()))
	{
		if (ExecuteSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExecuteSound, OwnerAgent->GetActorLocation());
		}
	}
	
	return true;
}
