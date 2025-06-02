#include "Brimstone_Q_Incendiary.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/Brimstone/IncendiaryBomb.h"
#include "AgentAbility/BaseGround.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UBrimstone_Q_Incendiary::UBrimstone_Q_Incendiary(): UBaseGameplayAbility()
{
    // 기본 설정
    FGameplayTagContainer Tags;
    Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
    SetAssetTags(Tags);
    
    m_AbilityID = 5002;
    
    ActivationType = EAbilityActivationType::WithPrepare;
    FollowUpInputType = EFollowUpInputType::LeftClick;
}

bool UBrimstone_Q_Incendiary::OnLeftClickInput()
{
    // 투사체 스폰 (포물선 궤적을 위해 위쪽 각도로)
    FRotator ThrowRotation = FRotator(-30.0f, 0.0f, 0.0f); // 30도 위쪽으로
    if (SpawnProjectile(FVector::ZeroVector, ThrowRotation))
    {
        UE_LOG(LogTemp, Warning, TEXT("Brimstone Q - Incendiary 투사체 생성 성공"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Brimstone Q - Incendiary 투사체 생성 실패"));
        // 실패 시 어빌리티 취소
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
    }
    return true;
}
