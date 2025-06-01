#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ValorantGameplayTags.h"
#include "Valorant/ResourceManager/ValorantGameType.h"
#include "AgentAbilitySystemComponent.generated.h"

class UValorantGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityStateChanged, FGameplayTag, StateTag);

UCLASS()
class VALORANT_API UAgentAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UAgentAbilitySystemComponent();
    
    // 초기화
    void InitializeByAgentData(int32 agentID);
    
    // 어빌리티 관리
    UFUNCTION(BlueprintCallable)
    void SetAgentAbility(int32 abilityID, int32 level);
    
    UFUNCTION(BlueprintCallable)
    void ResetAgentAbilities();
    
    // 어빌리티 정보 접근
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_C() const { return m_Ability_C; }
    
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_E() const { return m_Ability_E; }
    
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_Q() const { return m_Ability_Q; }
    
    UFUNCTION(BlueprintCallable)
    FAbilityData GetAbility_X() const { return m_Ability_X; }
    
    // 스킬 입력 처리
    UFUNCTION(BlueprintCallable)
    bool TrySkillInput(const FGameplayTag& InputTag);
    UFUNCTION(BlueprintCallable)
    bool TryActivateAbilityByTag(const FGameplayTag& InputTag);
    
    // 상태 조회 헬퍼
    UFUNCTION(BlueprintPure, Category = "Ability|State")
    bool IsAbilityActive() const;
    
    // 이벤트
    UPROPERTY(BlueprintAssignable)
    FOnAbilityStateChanged OnAbilityStateChanged;
    
    // 어빌리티 강제 정리
    UFUNCTION(BlueprintCallable)
    void ForceCleanupAllAbilities();

    // 네트워크 동기화 개선
    UFUNCTION(NetMulticast, Reliable)
    void MulticastRPC_OnAbilityExecuted(FGameplayTag AbilityTag, bool bSuccess);
    
protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // GameplayEvent 처리 오버라이드 - 후속 입력 처리용
    virtual int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) override;
    
private:
    UPROPERTY()
    UValorantGameInstance* m_GameInstance = nullptr;
    
    // 스킬 태그 집합
    TSet<FGameplayTag> SkillTags = {
        FValorantGameplayTags::Get().InputTag_Ability_C,
        FValorantGameplayTags::Get().InputTag_Ability_E,
        FValorantGameplayTags::Get().InputTag_Ability_Q,
        FValorantGameplayTags::Get().InputTag_Ability_X
    };
    
    // 어빌리티 데이터
    UPROPERTY(Replicated)
    int32 m_AgentID;
    
    UPROPERTY(Replicated)
    FAbilityData m_Ability_C;
    
    UPROPERTY(Replicated)
    FAbilityData m_Ability_E;
    
    UPROPERTY(Replicated)
    FAbilityData m_Ability_Q;
    
    UPROPERTY(Replicated)
    FAbilityData m_Ability_X;
    
    // 초기화 헬퍼
    void InitializeAttribute(const FAgentData* agentData);
    void RegisterAgentAbilities(const FAgentData* agentData);
};