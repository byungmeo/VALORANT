#include "Phoenix_Q_HotHands.h"
#include "PhoenixFireOrbActor.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AgentAbility/BaseProjectile.h"
#include "Player/Agent/BaseAgent.h"
#include "Kismet/GameplayStatics.h"

UPhoenix_Q_HotHands::UPhoenix_Q_HotHands() : UBaseGameplayAbility()
{
	// === 어빌리티 기본 설정 ===
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")));
	SetAssetTags(Tags);

	m_AbilityID = 2002;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::LeftOrRight;
	FollowUpTime = 15.0f; // 15초 대기 시간
}

void UPhoenix_Q_HotHands::WaitAbility()
{
	// 화염 오브 생성
	SpawnFireOrb();
}


bool UPhoenix_Q_HotHands::OnLeftClickInput()
{
	// 오브 정리
	DestroyFireOrb();

	// 직선 던지기로 설정
	CurrentThrowType = EPhoenixQThrowType::Straight;
	UpdateOrbThrowType(CurrentThrowType);

	// 투사체 생성
	bool bSuccess = SpawnProjectileByType(EPhoenixQThrowType::Straight);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 직선 투사체 생성 성공"));

		// 던지기 사운드 재생
		if (ThrowSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ThrowSound);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 직선 투사체 생성 실패"));
	}

	return bSuccess; // 성공 시 어빌리티 종료
}

bool UPhoenix_Q_HotHands::OnRightClickInput()
{
	// 오브 정리
	DestroyFireOrb();

	// 포물선 던지기로 설정
	CurrentThrowType = EPhoenixQThrowType::Curved;
	UpdateOrbThrowType(CurrentThrowType);

	// 투사체 생성
	bool bSuccess = SpawnProjectileByType(EPhoenixQThrowType::Curved);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("Phoenix Q - 포물선 투사체 생성 성공"));

		// 던지기 사운드 재생
		if (ThrowSound)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ThrowSound);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 포물선 투사체 생성 실패"));
	}

	return bSuccess; // 성공 시 어빌리티 종료
}

void UPhoenix_Q_HotHands::SpawnFireOrb()
{
	if (!FireOrbClass)
		return;

	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;

	// 서버에서 3인칭 오브 생성 (모든 클라이언트에 자동 복제)
	if (HasAuthority(&CurrentActivationInfo))
	{
		FVector HandLocation3P = OwnerAgent->GetMesh()->GetSocketLocation(FName("R_WeaponPoint"));
		FRotator HandRotation = OwnerAgent->GetControlRotation();

		FActorSpawnParameters SpawnParams3P;
		SpawnParams3P.Owner = OwnerAgent;
		SpawnParams3P.Instigator = OwnerAgent;
		SpawnParams3P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedFireOrb = GetWorld()->SpawnActor<APhoenixFireOrbActor>(
			FireOrbClass, HandLocation3P, HandRotation, SpawnParams3P);

		if (SpawnedFireOrb)
		{
			// 3인칭 오브로 설정 (복제되도록)
			SpawnedFireOrb->SetOrbViewType(EOrbViewType::ThirdPerson);

			// 3인칭 메쉬에 부착
			SpawnedFireOrb->AttachToComponent(OwnerAgent->GetMesh(),
			                                  FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			                                  FName("R_Hand"));
		}
	}

	// 로컬 플레이어인 경우에만 1인칭 오브 생성
	if (OwnerAgent->IsLocallyControlled())
	{
		FVector HandLocation1P = OwnerAgent->GetMesh1P()->GetSocketLocation(FName("R_WeaponPoint"));
		FRotator HandRotation = OwnerAgent->GetControlRotation();

		FActorSpawnParameters SpawnParams1P;
		SpawnParams1P.Owner = OwnerAgent;
		SpawnParams1P.Instigator = OwnerAgent;
		SpawnParams1P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedFireOrb1P = GetWorld()->SpawnActor<APhoenixFireOrbActor>(
			FireOrbClass, HandLocation1P, HandRotation, SpawnParams1P);

		if (SpawnedFireOrb1P)
		{
			// 1인칭 오브로 설정 (복제 안 됨)
			SpawnedFireOrb1P->SetOrbViewType(EOrbViewType::FirstPerson);

			// 복제 비활성화
			SpawnedFireOrb1P->SetReplicates(false);

			// 1인칭 메쉬에 부착
			SpawnedFireOrb1P->AttachToComponent(OwnerAgent->GetMesh1P(),
			                                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			                                    FName("R_WeaponPointSocket"));
		}
	}
}

void UPhoenix_Q_HotHands::DestroyFireOrb()
{
	// 3인칭 오브 제거 (서버에서만)
	if (HasAuthority(&CurrentActivationInfo) && SpawnedFireOrb)
	{
		SpawnedFireOrb->Destroy();
		SpawnedFireOrb = nullptr;
	}

	// 1인칭 오브 제거 (로컬에서만)
	if (SpawnedFireOrb1P)
	{
		SpawnedFireOrb1P->Destroy();
		SpawnedFireOrb1P = nullptr;
	}
}

void UPhoenix_Q_HotHands::UpdateOrbThrowType(EPhoenixQThrowType ThrowType)
{
	bool bIsCurved = (ThrowType == EPhoenixQThrowType::Curved);

	// 3인칭 오브 업데이트
	if (SpawnedFireOrb)
	{
		SpawnedFireOrb->SetThrowType(bIsCurved);
	}

	// 1인칭 오브 업데이트
	if (SpawnedFireOrb1P)
	{
		SpawnedFireOrb1P->SetThrowType(bIsCurved);
	}
}

bool UPhoenix_Q_HotHands::SpawnProjectileByType(EPhoenixQThrowType ThrowType)
{
	TSubclassOf<ABaseProjectile> ProjectileToSpawn = nullptr;

	switch (ThrowType)
	{
	case EPhoenixQThrowType::Straight:
		ProjectileToSpawn = StraightProjectileClass ? StraightProjectileClass : ProjectileClass;
		break;
	case EPhoenixQThrowType::Curved:
		ProjectileToSpawn = CurvedProjectileClass ? CurvedProjectileClass : ProjectileClass;
		break;
	default:
		ProjectileToSpawn = ProjectileClass;
		break;
	}

	if (ProjectileToSpawn == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Phoenix Q - 투사체 클래스가 설정되지 않음"));
		return false;
	}

	// 기존 SpawnProjectile 로직을 사용하되, 투사체 클래스만 교체
	TSubclassOf<ABaseProjectile> OriginalProjectileClass = ProjectileClass;
	ProjectileClass = ProjectileToSpawn;

	bool bResult = SpawnProjectile();

	// 원래 클래스로 복원
	ProjectileClass = OriginalProjectileClass;

	return bResult;
}

void UPhoenix_Q_HotHands::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                     bool bWasCancelled)
{
	// 오브 정리
	DestroyFireOrb();
	// 상태 초기화
	CurrentThrowType = EPhoenixQThrowType::None;

	// 부모 클래스 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
