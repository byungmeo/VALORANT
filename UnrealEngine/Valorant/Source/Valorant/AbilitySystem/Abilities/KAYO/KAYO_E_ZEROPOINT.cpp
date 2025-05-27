#include "KAYO_E_ZEROPOINT.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AgentAbility/KayO/KayoKnife.h"
#include "GameFramework/Character.h"

UKAYO_E_ZEROPOINT::UKAYO_E_ZEROPOINT(): UBaseGameplayAbility()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")));
	SetAssetTags(Tags);
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	m_AbilityID = 3003;
	
	// ZERO/point는 준비 후 클릭으로 던지기
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftClick;
}

void UKAYO_E_ZEROPOINT::PrepareAbility()
{
	Super::PrepareAbility();
	
	// 나이프 장착 시 추가 로직이 필요하면 여기에 구현
	UE_LOG(LogTemp, Warning, TEXT("KAYO E - ZERO/point 준비 중"));
}

bool UKAYO_E_ZEROPOINT::OnLeftClickInput()
{
	// 좌클릭으로 나이프 던지기
	return ThrowKnife();
}

bool UKAYO_E_ZEROPOINT::ThrowKnife()
{
	if (!HasAuthority(&CurrentActivationInfo) || !ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO E - 권한 없음 또는 투사체 클래스 없음"));
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO E - 캐릭터 참조 실패"));
		return false;
	}

	// 나이프 스폰
	FVector SpawnLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f;
	FRotator SpawnRotation = Character->GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AKayoKnife* Knife = GetWorld()->SpawnActor<AKayoKnife>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (Knife)
	{
		// 나이프 던지기 활성화
		Knife->OnThrow();
		SpawnedProjectile = Knife;
		
		UE_LOG(LogTemp, Warning, TEXT("KAYO E - 억제 나이프 생성 및 던지기 성공"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KAYO E - 억제 나이프 생성 실패"));
		return false;
	}
}