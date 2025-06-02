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
    
	if (FlashOverlay)
	{
		// 전체 화면 오버레이 투명도 설정
		FlashOverlay->SetRenderOpacity(CurrentFlashIntensity);
		FlashOverlay->SetColorAndOpacity(CurrentFlashColor);
	}

	// 방사형 효과 처리
	if (RadialFlashImage)
	{
		// 새로운 섬광이 시작될 때만 위치 계산 (이전 강도가 0이고 현재 강도가 0보다 클 때)
		if (PreviousIntensity <= 0.01f && CurrentFlashIntensity > 0.01f && !FlashWorldLocation.IsZero())
		{
			// 월드 좌표를 화면 좌표로 변환
			FixedFlashScreenPosition = ConvertWorldToScreenPosition(FlashWorldLocation);
			bFlashPositionFixed = true;
			
			UE_LOG(LogTemp, Warning, TEXT("섬광 위치 고정: X=%.2f, Y=%.2f"), FixedFlashScreenPosition.X, FixedFlashScreenPosition.Y);
		}
		
		// 고정된 위치가 있으면 그 위치 사용
		if (bFlashPositionFixed)
		{
			// 방사형 이미지 위치 업데이트
			UpdateRadialImagePosition(FixedFlashScreenPosition);
			
			// 방사형 이미지 투명도 설정 (전체 오버레이보다 약간 더 강하게)
			float RadialOpacity = FMath::Min(CurrentFlashIntensity * 1.2f, 1.0f);
			RadialFlashImage->SetRenderOpacity(RadialOpacity);
			RadialFlashImage->SetColorAndOpacity(CurrentFlashColor);
		}
		else
		{
			// 위치 정보가 없으면 화면 중앙에 배치
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RadialFlashImage->Slot))
			{
				CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			}
			
			RadialFlashImage->SetRenderOpacity(CurrentFlashIntensity);
			RadialFlashImage->SetColorAndOpacity(CurrentFlashColor);
		}
		
		// 섬광이 완전히 끝났을 때 위치 초기화
		if (CurrentFlashIntensity <= 0.01f && bFlashPositionFixed)
		{
			bFlashPositionFixed = false;
			FixedFlashScreenPosition = FVector2D::ZeroVector;
			UE_LOG(LogTemp, Warning, TEXT("섬광 위치 초기화"));
		}
	}
}

void UFlashWidget::StartFlashEffect(float Duration, EFlashType FlashType)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	
	// 새로운 섬광 시작 시 위치 고정 초기화
	bFlashPositionFixed = false;
	FixedFlashScreenPosition = FVector2D::ZeroVector;
	
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
	
	// 고정된 위치 초기화
	bFlashPositionFixed = false;
	FixedFlashScreenPosition = FVector2D::ZeroVector;
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
		
		// 화면 중앙을 (0,0)으로 하는 좌표계로 변환
		ScreenPosition.X = ScreenPosition.X - (ViewportSizeX * 0.5f);
		ScreenPosition.Y = ScreenPosition.Y - (ViewportSizeY * 0.5f);
	}
	else
	{
		// 프로젝션 실패 시 화면 중앙
		ScreenPosition = FVector2D::ZeroVector;
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
		
		// 화면보다 큰 이미지 크기 설정 (화면을 완전히 채우기 위해)
		float LargerDimension = FMath::Max(ViewportSizeX, ViewportSizeY);
		FVector2D ImageSize = FVector2D(LargerDimension * RadialImageSizeMultiplier, LargerDimension * RadialImageSizeMultiplier);
		
		// 위치 설정 (클램핑 없이 자유롭게 이동)
		CanvasSlot->SetPosition(ScreenPosition);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetSize(ImageSize);
		
		// 앵커를 중앙으로 설정
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	}
}