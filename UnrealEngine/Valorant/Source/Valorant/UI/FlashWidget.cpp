#include "FlashWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"

void UFlashWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	if (FlashOverlay)
	{
		// 처음에는 투명하게
		FlashOverlay->SetRenderOpacity(0.0f);
		FlashOverlay->SetColorAndOpacity(FLinearColor::White);
	}

	if (RadialFlashImage)
	{
		// 방사형 이미지도 처음에는 투명하게
		RadialFlashImage->SetRenderOpacity(0.0f);
		RadialFlashImage->SetColorAndOpacity(FLinearColor::White);
		
		// 기본 텍스처 설정
		if (DefaultRadialTexture)
		{
			RadialFlashImage->SetBrushFromTexture(DefaultRadialTexture);
		}
		
		// 캔버스 슬롯 초기 설정
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RadialFlashImage->Slot))
		{
			// 앵커를 중앙으로 설정
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			
			// 뷰포트 크기 가져오기
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				int32 ViewportSizeX, ViewportSizeY;
				PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
				
				// 화면보다 큰 초기 크기 설정
				float LargerDimension = FMath::Max(ViewportSizeX, ViewportSizeY);
				FVector2D InitialSize = FVector2D(LargerDimension * RadialImageSizeMultiplier, LargerDimension * RadialImageSizeMultiplier);
				CanvasSlot->SetSize(InitialSize);
			}
		}
	}

	CurrentFlashType = EFlashType::Default;
}

void UFlashWidget::UpdateFlashIntensity(float Intensity, FVector FlashWorldLocation)
{
	// 이전 강도 저장
	float PreviousIntensity = CurrentFlashIntensity;
	CurrentFlashIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
    
	// 페이드 효과 적용 (발로란트 스타일)
	float FadeIntensity = CalculateFadeIntensity(CurrentFlashIntensity);
	
	if (FlashOverlay)
	{
		// 전체 화면 오버레이 투명도 설정 (페이드 효과 적용)
		FlashOverlay->SetRenderOpacity(FadeIntensity);
		FlashOverlay->SetColorAndOpacity(CurrentFlashColor);
	}

	// 방사형 효과 처리
	if (RadialFlashImage)
	{
		// 새로운 섬광이 시작될 때 월드 위치 저장
		if (PreviousIntensity <= 0.01f && CurrentFlashIntensity > 0.01f && !FlashWorldLocation.IsZero())
		{
			StoredFlashWorldLocation = FlashWorldLocation;
			bHasStoredWorldLocation = true;

			// 저장된 월드 위치를 현재 카메라 기준으로 화면 좌표로 변환
			FixedFlashScreenPosition = ConvertWorldToScreenPosition(StoredFlashWorldLocation);
			 
			
			UE_LOG(LogTemp, Warning, TEXT("섬광 월드 위치 저장: %s"), *StoredFlashWorldLocation.ToString());
		}
		
		// 섬광 효과가 진행 중일 때
		if (CurrentFlashIntensity > 0.01f && bHasStoredWorldLocation)
		{
			UpdateRadialImagePosition(FixedFlashScreenPosition);
			
			// 방사형 이미지 투명도 설정 (페이드 효과 적용)
			float RadialOpacity = FMath::Min(FadeIntensity * 1.2f, 1.0f); // 방사형은 약간 더 강하게
			RadialFlashImage->SetRenderOpacity(RadialOpacity);
			RadialFlashImage->SetColorAndOpacity(CurrentFlashColor);
		}
		else if (CurrentFlashIntensity <= 0.01f)
		{
			// 섬광이 끝났을 때 초기화
			if (bHasStoredWorldLocation)
			{
				bHasStoredWorldLocation = false;
				StoredFlashWorldLocation = FVector::ZeroVector;
				UE_LOG(LogTemp, Warning, TEXT("섬광 위치 초기화"));
			}
			
			// 화면 중앙으로 리셋
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RadialFlashImage->Slot))
			{
				CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			}
			
			RadialFlashImage->SetRenderOpacity(0.0f);
		}
	}
}

void UFlashWidget::StartFlashEffect(float Duration, EFlashType FlashType)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	
	// 섬광 타입 설정
	SetFlashType(FlashType);
}

void UFlashWidget::StopFlashEffect()
{
	SetVisibility(ESlateVisibility::Collapsed);
	
	if (FlashOverlay)
	{
		FlashOverlay->SetRenderOpacity(0.0f);
	}
	
	if (RadialFlashImage)
	{
		RadialFlashImage->SetRenderOpacity(0.0f);
	}
	
	// 저장된 위치 초기화
	bHasStoredWorldLocation = false;
	StoredFlashWorldLocation = FVector::ZeroVector;
}

void UFlashWidget::SetFlashType(EFlashType InFlashType)
{
	CurrentFlashType = InFlashType;
	CurrentFlashColor = GetColorForFlashType(InFlashType);
	
	// 텍스처 변경
	if (RadialFlashImage)
	{
		UTexture2D* NewTexture = GetTextureForFlashType(InFlashType);
		if (NewTexture)
		{
			RadialFlashImage->SetBrushFromTexture(NewTexture);
		}
	}
}

FVector2D UFlashWidget::ConvertWorldToScreenPosition(FVector WorldLocation)
{
	FVector2D ScreenPosition;
	
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPosition))
	{
		// 뷰포트 크기 가져오기
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
		
		// 화면 좌표를 -1 ~ 1 범위로 정규화
		ScreenPosition.X = ((ScreenPosition.X / ViewportSizeX) - 0.5f) * 2.0f;
		ScreenPosition.Y = ((ScreenPosition.Y / ViewportSizeY) - 0.5f) * 2.0f;
		
		// UI 좌표계로 변환 (픽셀 단위)
		ScreenPosition.X *= ViewportSizeX * 0.5f;
		ScreenPosition.Y *= ViewportSizeY * 0.5f;
		
		// 화면 밖으로 나가는 것을 제한
		float MaxOffset = FMath::Max(ViewportSizeX, ViewportSizeY) * 0.8f;
		ScreenPosition.X = FMath::Clamp(ScreenPosition.X, -MaxOffset, MaxOffset);
		ScreenPosition.Y = FMath::Clamp(ScreenPosition.Y, -MaxOffset, MaxOffset);
	}
	else
	{
		// 화면 밖이거나 뒤에 있는 경우 처리
		if (PC)
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
			
			FVector ToFlash = (WorldLocation - CameraLocation).GetSafeNormal();
			FVector CameraForward = CameraRotation.Vector();
			
			float DotProduct = FVector::DotProduct(CameraForward, ToFlash);
			if (DotProduct < 0) // 뒤에 있는 경우
			{
				// 화면 중앙으로 설정
				ScreenPosition = FVector2D::ZeroVector;
			}
		}
	}
	
	return ScreenPosition;
}

FLinearColor UFlashWidget::GetColorForFlashType(EFlashType FlashType)
{
	switch (FlashType)
	{
	case EFlashType::Phoenix:
		return PhoenixFlashColor;
	case EFlashType::KayO:
		return KayOFlashColor;
	default:
		return DefaultFlashColor;
	}
}

UTexture2D* UFlashWidget::GetTextureForFlashType(EFlashType FlashType)
{
	switch (FlashType)
	{
	case EFlashType::Phoenix:
		return PhoenixRadialTexture ? PhoenixRadialTexture : DefaultRadialTexture;
	case EFlashType::KayO:
		return KayORadialTexture ? KayORadialTexture : DefaultRadialTexture;
	default:
		return DefaultRadialTexture;
	}
}

void UFlashWidget::UpdateRadialImagePosition(FVector2D ScreenPosition)
{
	if (!RadialFlashImage)
		return;
	
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RadialFlashImage->Slot))
	{
		// 뷰포트 크기 가져오기
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (!PC) return;
		
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
		
		// 화면보다 큰 이미지 크기 설정
		float LargerDimension = FMath::Max(ViewportSizeX, ViewportSizeY);
		FVector2D ImageSize = FVector2D(LargerDimension * RadialImageSizeMultiplier, LargerDimension * RadialImageSizeMultiplier);
		
		// 앵커를 중앙으로 설정
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		
		// 위치 설정
		CanvasSlot->SetPosition(ScreenPosition);
		CanvasSlot->SetSize(ImageSize);
		
		// 디버그용
		UE_LOG(LogTemp, VeryVerbose, TEXT("방사형 이미지 위치 업데이트: X=%.2f, Y=%.2f"), ScreenPosition.X, ScreenPosition.Y);
	}
}

float UFlashWidget::CalculateFadeIntensity(float BaseIntensity)
{
	// 발로란트 스타일 페이드 효과
	if (BaseIntensity >= 0.95f)
	{
		// 완전 실명 상태 - 즉시 최대 밝기
		return 1.0f;
	}
	else if (BaseIntensity > 0.01f)
	{
		// 회복 중 - 빠른 초기 감소, 느린 후기 감소
		// Pow 값을 조정하여 페이드 커브 변경 가능
		return FMath::Pow(BaseIntensity, 2.5f);
	}
	else
	{
		return 0.0f;
	}
}