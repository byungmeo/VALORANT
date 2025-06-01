#include "AgentAbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "Valorant.h"
#include "Abilities/BaseGameplayAbility.h"
#include "Attributes/BaseAttributeSet.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Net/UnrealNetwork.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Valorant/Player/Agent/BaseAgent.h"

UAgentAbilitySystemComponent::UAgentAbilitySystemComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Tick 불필요
    SetIsReplicated(true);
}

void UAgentAbilitySystemComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 스킬 태그 초기화
    auto& tagManager = UGameplayTagsManager::Get();
    SkillTags.Add(tagManager.RequestGameplayTag("Input.Skill.Q"));
    SkillTags.Add(tagManager.RequestGameplayTag("Input.Skill.E"));
    SkillTags.Add(tagManager.RequestGameplayTag("Input.Skill.X"));
    SkillTags.Add(tagManager.RequestGameplayTag("Input.Skill.C"));
}

void UAgentAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UAgentAbilitySystemComponent, m_AgentID);
    DOREPLIFETIME(UAgentAbilitySystemComponent, m_Ability_C);
    DOREPLIFETIME(UAgentAbilitySystemComponent, m_Ability_E);
    DOREPLIFETIME(UAgentAbilitySystemComponent, m_Ability_Q);
    DOREPLIFETIME(UAgentAbilitySystemComponent, m_Ability_X);
}

void UAgentAbilitySystemComponent::InitializeByAgentData(int32 agentID)
{
    if (!GetOwner()->HasAuthority())
    {
        NET_LOG(LogTemp, Error, TEXT("InitializeByAgentData는 서버에서만 호출되어야 합니다"));
        return;
    }
    
    m_GameInstance = Cast<UValorantGameInstance>(GetWorld()->GetGameInstance());
    FAgentData* data = m_GameInstance->GetAgentData(agentID);
    
    if (!data)
    {
        NET_LOG(LogTemp, Error, TEXT("AgentData를 찾을 수 없습니다. ID: %d"), agentID);
        return;
    }
    
    InitializeAttribute(data);
    RegisterAgentAbilities(data);
}

void UAgentAbilitySystemComponent::InitializeAttribute(const FAgentData* agentData)
{
    SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), agentData->BaseHealth);
    SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), agentData->MaxHealth);
    SetNumericAttributeBase(UBaseAttributeSet::GetArmorAttribute(), agentData->BaseArmor);
    SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorAttribute(), agentData->MaxArmor);
}

void UAgentAbilitySystemComponent::RegisterAgentAbilities(const FAgentData* agentData)
{
    NET_LOG(LogTemp, Warning, TEXT("어빌리티 등록: C(%d), E(%d), Q(%d), X(%d)"), 
            agentData->AbilityID_C, agentData->AbilityID_E, 
            agentData->AbilityID_Q, agentData->AbilityID_X);
    
    SetAgentAbility(agentData->AbilityID_C, 1);
    SetAgentAbility(agentData->AbilityID_E, 1);
    SetAgentAbility(agentData->AbilityID_Q, 1);
    SetAgentAbility(agentData->AbilityID_X, 1);
}

void UAgentAbilitySystemComponent::SetAgentAbility(int32 abilityID, int32 level)
{
    if (!m_GameInstance)
    {
        NET_LOG(LogTemp, Error, TEXT("GameInstance가 없습니다"));
        return;
    }
    
    FAbilityData* abilityData = m_GameInstance->GetAbilityData(abilityID);
    if (!abilityData || !abilityData->AbilityClass)
    {
        NET_LOG(LogTemp, Error, TEXT("어빌리티 데이터를 찾을 수 없습니다. ID: %d"), abilityID);
        return;
    }
    
    TSubclassOf<UGameplayAbility> abilityClass = abilityData->AbilityClass;
    UGameplayAbility* ga = abilityClass->GetDefaultObject<UGameplayAbility>();
    const FGameplayTagContainer& tagCon = ga->GetAssetTags();
    
    if (tagCon.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("어빌리티 [%s]에 태그가 없습니다"), *GetNameSafe(ga));
        return;
    }
    
    // 스킬 슬롯 확인 및 데이터 저장
    bool bIsSkill = false;
    FGameplayTag skillTag;
    
    for (const FGameplayTag& tag : tagCon)
    {
        if (SkillTags.Contains(tag))
        {
            skillTag = tag;
            bIsSkill = true;
            
            // 데이터 저장
            if (tag == FValorantGameplayTags::Get().InputTag_Ability_C)
                m_Ability_C = *abilityData;
            else if (tag == FValorantGameplayTags::Get().InputTag_Ability_E)
                m_Ability_E = *abilityData;
            else if (tag == FValorantGameplayTags::Get().InputTag_Ability_Q)
                m_Ability_Q = *abilityData;
            else if (tag == FValorantGameplayTags::Get().InputTag_Ability_X)
                m_Ability_X = *abilityData;
            
            break;
        }
    }
    
    if (!bIsSkill)
    {
        UE_LOG(LogTemp, Error, TEXT("%s는 스킬 태그가 없습니다"), *abilityClass->GetName());
        return;
    }
    
    // 어빌리티 부여
    FGameplayAbilitySpec spec(abilityClass, level);
    spec.GetDynamicSpecSourceTags().AddTag(skillTag);
    GiveAbility(spec);
    
    NET_LOG(LogTemp, Warning, TEXT("%s 스킬 등록 완료"), *skillTag.ToString());
}

void UAgentAbilitySystemComponent::ResetAgentAbilities()
{
    NET_LOG(LogTemp, Warning, TEXT("모든 어빌리티 리셋"));
    
    // 활성 어빌리티 강제 취소
    ForceCleanupAllAbilities();
    
    // 모든 어빌리티 제거
    for (const FGameplayAbilitySpec& spec : GetActivatableAbilities())
    {
        ClearAbility(spec.Handle);
    }
}

bool UAgentAbilitySystemComponent::TrySkillInput(const FGameplayTag& InputTag)
{
    return TryActivateAbilityByTag(InputTag);
}

bool UAgentAbilitySystemComponent::TryActivateAbilityByTag(const FGameplayTag& InputTag)
{
    // 활성 어빌리티가 있는지 확인
    if (HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting))
    {
        // 후속 입력 처리를 위해 GameplayEvent 전송
        FGameplayEventData EventData;
        EventData.EventTag = InputTag;
        HandleGameplayEvent(InputTag, &EventData);
        return true;
    }
    
    // 일반 어빌리티 활성화
    FGameplayTagContainer TagContainer(InputTag);
    return TryActivateAbilitiesByTag(TagContainer);
}

bool UAgentAbilitySystemComponent::IsAbilityActive() const
{
    // 어빌리티 관련 상태 태그 확인
    return HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing) ||
           HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting) ||
           HasMatchingGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
}

void UAgentAbilitySystemComponent::ForceCleanupAllAbilities()
{
    NET_LOG(LogTemp, Warning, TEXT("모든 어빌리티 강제 정리"));
    
    // 모든 활성 어빌리티 취소
    TArray<FGameplayAbilitySpec*> ActiveSpecs;
    for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        if (Spec.IsActive())
        {
            ActiveSpecs.Add(&Spec);
        }
    }
    
    for (FGameplayAbilitySpec* Spec : ActiveSpecs)
    {
        if (Spec && Spec->Ability)
        {
            NET_LOG(LogTemp, Warning, TEXT("어빌리티 강제 취소: %s"), *Spec->Ability->GetName());
            CancelAbilitySpec(*Spec, nullptr);
        }
    }
    
    // 모든 어빌리티 관련 태그 제거
    RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Preparing);
    RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Waiting);
    RemoveLooseGameplayTag(FValorantGameplayTags::Get().State_Ability_Executing);
    RemoveLooseGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch);
    
    // 상태 변경 알림
    OnAbilityStateChanged.Broadcast(FGameplayTag());
}

int32 UAgentAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
    int32 Result = Super::HandleGameplayEvent(EventTag, Payload);
    
    // 활성 어빌리티에 이벤트 전달
    for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        if (Spec.IsActive())
        {
            if (UBaseGameplayAbility* Ability = Cast<UBaseGameplayAbility>(Spec.GetPrimaryInstance()))
            {
                // 후속 입력 처리
                Ability->HandleFollowUpInput(EventTag);
            }
        }
    }
    
    return Result;
}

void UAgentAbilitySystemComponent::MulticastRPC_OnAbilityExecuted_Implementation(FGameplayTag AbilityTag, bool bSuccess)
{
    // 클라이언트에서 어빌리티 실행 결과 처리
    if (!GetOwner()->HasAuthority())
    {
        if (bSuccess)
        {
            NET_LOG(LogTemp, Display, TEXT("어빌리티 실행 성공: %s"), *AbilityTag.ToString());
        }
        else
        {
            NET_LOG(LogTemp, Warning, TEXT("어빌리티 실행 실패: %s"), *AbilityTag.ToString());
        }
        
        OnAbilityStateChanged.Broadcast(AbilityTag);
    }
}