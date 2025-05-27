#include "Sage_C_BarrierOrb.h"

#include "BarrierOrbActor.h"
#include "BarrierWallActor.h"
#include "AbilitySystem/ValorantGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Player/Agent/BaseAgent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"

USage_C_BarrierOrb::USage_C_BarrierOrb()
{
	FGameplayTagContainer Tags;
	Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")));
	SetAssetTags(Tags);
	
	m_AbilityID = 1001;
	ActivationType = EAbilityActivationType::WithPrepare;
	FollowUpInputType = EFollowUpInputType::Any;  // 좌클릭, 우클릭, 스크롤 모두 사용
	FollowUpTime = 30.0f;  // 30초 동안 대기
}

void USage_C_BarrierOrb::WaitAbility()
{
	// 장벽 오브 생성
	SpawnBarrierOrb();
	
	// 미리보기 액터 생성
	CreatePreviewActors();
	
	// 미리보기 업데이트 타이머 시작
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(PreviewUpdateTimer, this, 
			&USage_C_BarrierOrb::UpdateBarrierPreview, 0.01f, true);
	}
}

bool USage_C_BarrierOrb::OnLeftClickInput()
{
	// 현재 위치가 유효한지 확인
	if (!bIsValidPlacement)
		return false;
	
	// 장벽 설치
	FVector PlaceLocation = GetBarrierPlaceLocation();
	FRotator PlaceRotation = FRotator(0.f, CurrentRotation, 0.f);

	// 실제 장벽 스폰
	SpawnBarrierWall(PlaceLocation, PlaceRotation);
	
	// 설치 이펙트 및 사운드
	if (PlaceEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PlaceEffect, PlaceLocation);
	}
	
	if (PlaceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PlaceSound, PlaceLocation);
	}
	
	return true;  // 어빌리티 종료
}

bool USage_C_BarrierOrb::OnRightClickInput()
{
	// 시계 방향으로 회전
	RotateBarrier(true);
	return false;  // 어빌리티를 종료하지 않음
}

bool USage_C_BarrierOrb::OnRepeatInput()
{
	// 반시계 방향으로 회전 (스크롤 입력)
	RotateBarrier(false);
	return false;  // 어빌리티를 종료하지 않음
}

void USage_C_BarrierOrb::SpawnBarrierOrb()
{
	if (!HasAuthority(&CurrentActivationInfo) || !BarrierOrbClass)
		return;
	
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return;
	
	// 오브 스폰 위치 계산 (손 위치)
	FVector HandLocation = OwnerAgent->GetMesh()->GetSocketLocation(FName("hand_r"));
	FRotator HandRotation = OwnerAgent->GetControlRotation();
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerAgent;
	SpawnParams.Instigator = OwnerAgent;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	SpawnedBarrierOrb = GetWorld()->SpawnActor<ABarrierOrbActor>(
		BarrierOrbClass, HandLocation, HandRotation, SpawnParams);
	
	if (SpawnedBarrierOrb)
	{
		// 오브를 손에 부착
		SpawnedBarrierOrb->AttachToComponent(OwnerAgent->GetMesh(), 
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_r"));
	}
}

void USage_C_BarrierOrb::DestroyBarrierOrb()
{
	if (SpawnedBarrierOrb)
	{
		SpawnedBarrierOrb->Destroy();
		SpawnedBarrierOrb = nullptr;
	}
}

void USage_C_BarrierOrb::UpdateBarrierPreview()
{
	FVector PlaceLocation = GetBarrierPlaceLocation();
	FRotator PlaceRotation = FRotator(0.f, CurrentRotation, 0.f);
	
	// 설치 가능 여부 확인
	bool bValid = IsValidPlacement(PlaceLocation, PlaceRotation);
	if (bValid != bIsValidPlacement)
	{
		bIsValidPlacement = bValid;
		UpdatePreviewMaterial(bValid);
		
		// 오브 상태 업데이트
		if (SpawnedBarrierOrb)
		{
			SpawnedBarrierOrb->SetPlacementValid(bValid);
		}
	}
	
	// 미리보기 장벽 위치 업데이트
	if (PreviewBarrierWall)
	{
		PreviewBarrierWall->SetActorLocationAndRotation(PlaceLocation, PlaceRotation);
	}
}

void USage_C_BarrierOrb::RotateBarrier(bool bClockwise)
{
	float RotationDelta = bClockwise ? RotationStep : -RotationStep;
	CurrentRotation += RotationDelta;
	
	// 0-360도 범위로 정규화
	CurrentRotation = FMath::Fmod(CurrentRotation + 360.f, 360.f);
	
	// 회전 사운드 재생
	if (RotateSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), RotateSound);
	}
	
	// 즉시 미리보기 업데이트
	UpdateBarrierPreview();
}

bool USage_C_BarrierOrb::IsValidPlacement(FVector Location, FRotator Rotation)
{
	if (!GetWorld())
		return false;
	
	// 3개의 세그먼트 모두 확인
	TArray<FVector> SegmentLocations;
	SegmentLocations.Add(Location);  // 중앙
	
	FVector RightOffset = Rotation.RotateVector(FVector(0, BarrierSegmentSize.Y + 10.f, 0));
	FVector LeftOffset = Rotation.RotateVector(FVector(0, -(BarrierSegmentSize.Y + 10.f), 0));
	
	SegmentLocations.Add(Location + RightOffset);  // 오른쪽
	SegmentLocations.Add(Location + LeftOffset);   // 왼쪽
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CachedActorInfo.AvatarActor.Get());
	if (PreviewBarrierWall)
	{
		QueryParams.AddIgnoredActor(PreviewBarrierWall);
	}
	
	for (const FVector& SegLoc : SegmentLocations)
	{
		// 지면 확인
		FHitResult GroundHit;
		FVector TraceStart = SegLoc + FVector(0, 0, 50.f);
		FVector TraceEnd = SegLoc - FVector(0, 0, 200.f);
		
		bool bHitGround = GetWorld()->LineTraceSingleByChannel(
			GroundHit,
			TraceStart,
			TraceEnd,
			ECC_WorldStatic,
			QueryParams
		);
		
		if (!bHitGround)
			return false;
		
		// 경사도 확인 (너무 가파른 경사면 불가)
		float SlopeDot = FVector::DotProduct(GroundHit.Normal, FVector::UpVector);
		if (SlopeDot < 0.7f)  // ~45도 이상 경사
			return false;
		
		// 장애물 확인
		FVector BoxExtent = BarrierSegmentSize * 0.5f;
		FVector BoxLocation = SegLoc + FVector(0, 0, BoxExtent.Z);
		
		bool bBlocked = GetWorld()->OverlapAnyTestByChannel(
			BoxLocation,
			Rotation.Quaternion(),
			ECC_WorldStatic,
			FCollisionShape::MakeBox(BoxExtent),
			QueryParams
		);
		
		if (bBlocked)
			return false;
	}
	
	return true;
}

FVector USage_C_BarrierOrb::GetBarrierPlaceLocation()
{
	ABaseAgent* OwnerAgent = Cast<ABaseAgent>(CachedActorInfo.AvatarActor.Get());
	if (!OwnerAgent)
		return FVector::ZeroVector;
	
	// 카메라에서 레이캐스트
	FVector CameraLocation = OwnerAgent->Camera->GetComponentLocation();
	FVector CameraForward = OwnerAgent->Camera->GetForwardVector();
	
	FHitResult HitResult;
	FVector TraceEnd = CameraLocation + CameraForward * MaxPlaceDistance;
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerAgent);
	if (PreviewBarrierWall)
	{
		QueryParams.AddIgnoredActor(PreviewBarrierWall);
	}
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CameraLocation,
		TraceEnd,
		ECC_WorldStatic,
		QueryParams
	);
	
	FVector PlaceLocation;
	if (bHit)
	{
		PlaceLocation = HitResult.Location;
	}
	else
	{
		PlaceLocation = TraceEnd;
	}
	
	// 지면에 스냅
	FHitResult GroundHit;
	FVector GroundTraceStart = PlaceLocation + FVector(0, 0, 100.f);
	FVector GroundTraceEnd = PlaceLocation - FVector(0, 0, 500.f);
	
	if (GetWorld()->LineTraceSingleByChannel(
		GroundHit,
		GroundTraceStart,
		GroundTraceEnd,
		ECC_WorldStatic,
		QueryParams))
	{
		PlaceLocation = GroundHit.Location;
	}
	
	return PlaceLocation;
}

void USage_C_BarrierOrb::SpawnBarrierWall(FVector Location, FRotator Rotation)
{
	if (!HasAuthority(&CurrentActivationInfo) || !BarrierWallClass)
		return;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = CachedActorInfo.AvatarActor.Get();
	SpawnParams.Instigator = Cast<APawn>(CachedActorInfo.AvatarActor.Get());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ABarrierWallActor* BarrierWall = GetWorld()->SpawnActor<ABarrierWallActor>(
		BarrierWallClass, Location, Rotation, SpawnParams);
	
	if (BarrierWall)
	{
		// 장벽 설정 초기화
		BarrierWall->InitializeBarrier(BarrierHealth, BarrierLifespan, BarrierBuildTime);
		
		// 건설 사운드
		if (BuildSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), BuildSound, Location);
		}
	}
}

void USage_C_BarrierOrb::CreatePreviewActors()
{
	if (!GetWorld() || !BarrierWallClass)
		return;
	
	// BarrierWallActor를 미리보기 모드로 생성
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = CachedActorInfo.AvatarActor.Get();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	PreviewBarrierWall = GetWorld()->SpawnActor<ABarrierWallActor>(BarrierWallClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	
	if (PreviewBarrierWall)
	{
		// 미리보기 모드 설정
		PreviewBarrierWall->SetPreviewMode(true);
		
		// 충돌 비활성화
		PreviewBarrierWall->SetActorEnableCollision(false);
	}
}

void USage_C_BarrierOrb::DestroyPreviewActors()
{
	if (PreviewBarrierWall)
	{
		PreviewBarrierWall->Destroy();
		PreviewBarrierWall = nullptr;
	}
}

void USage_C_BarrierOrb::UpdatePreviewMaterial(bool bValid)
{
	if (!PreviewBarrierWall)
		return;
	
	PreviewBarrierWall->SetPlacementValid(bValid);
}

void USage_C_BarrierOrb::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
                                    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PreviewUpdateTimer);
	}
	
	// 오브 및 미리보기 정리
	DestroyBarrierOrb();
	DestroyPreviewActors();
	
	// 상태 초기화
	CurrentRotation = 0.f;
	bIsValidPlacement = false;
	
	// 부모 클래스 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}