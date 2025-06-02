#include "SageHealCalculation.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"

USageHealCalculation::USageHealCalculation()
{
    // 필요한 어트리뷰트 캡처
    RelevantAttributesToCapture.Add(FGameplayEffectAttributeCaptureDefinition(
        FGameplayAttribute(FindFieldChecked<FProperty>(UBaseAttributeSet::StaticClass(), 
        GET_MEMBER_NAME_CHECKED(UBaseAttributeSet, Health))),
        EGameplayEffectAttributeCaptureSource::Target,
        false
    ));
    
    RelevantAttributesToCapture.Add(FGameplayEffectAttributeCaptureDefinition(
        FGameplayAttribute(FindFieldChecked<FProperty>(UBaseAttributeSet::StaticClass(), 
        GET_MEMBER_NAME_CHECKED(UBaseAttributeSet, MaxHealth))),
        EGameplayEffectAttributeCaptureSource::Target,
        false
    ));
}

void USageHealCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, 
                                                 FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    
    // 소스와 타겟 태그 가져오기
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
    
    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;
    
    // 현재 체력과 최대 체력 가져오기
    float CurrentHealth = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        FGameplayEffectAttributeCaptureDefinition(
            FGameplayAttribute(FindFieldChecked<FProperty>(UBaseAttributeSet::StaticClass(), 
            GET_MEMBER_NAME_CHECKED(UBaseAttributeSet, Health))),
            EGameplayEffectAttributeCaptureSource::Target,
            false
        ), 
        EvaluationParameters, 
        CurrentHealth
    );
    
    float MaxHealth = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        FGameplayEffectAttributeCaptureDefinition(
            FGameplayAttribute(FindFieldChecked<FProperty>(UBaseAttributeSet::StaticClass(), 
            GET_MEMBER_NAME_CHECKED(UBaseAttributeSet, MaxHealth))),
            EGameplayEffectAttributeCaptureSource::Target,
            false
        ), 
        EvaluationParameters, 
        MaxHealth
    );
    
    // 힐링량 계산
    float HealAmount = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Heal.Amount")), false, 0.0f);
    
    // 틱당 힐링량 계산 (매 0.1초마다)
    float HealPerTick = HealAmount / 50.0f;  // 5초 동안 50번 (0.1초마다)
    
    // 오버힐 방지
    float ActualHeal = FMath::Min(HealPerTick, MaxHealth - CurrentHealth);
    
    if (ActualHeal > 0.0f)
    {
        // 힐링 적용
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(
                FGameplayAttribute(FindFieldChecked<FProperty>(UBaseAttributeSet::StaticClass(), 
                GET_MEMBER_NAME_CHECKED(UBaseAttributeSet, Health))),
                EGameplayModOp::Additive,
                ActualHeal
            )
        );
    }
}