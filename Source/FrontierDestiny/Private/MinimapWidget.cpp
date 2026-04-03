// Fill out your copyright notice in the Description page of Project Settings.

#include "MinimapWidget.h"
#include "ReconManagerSubsystem.h"
#include "CacheManagerSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/Pawn.h"

void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	DynamicMinimapMaterial = UMaterialInstanceDynamic::Create(BaseMinimapMaterial, this);
	MinimapImage->SetBrushFromMaterial(DynamicMinimapMaterial);

	UReconManagerSubsystem* MinimapManager = GetGameInstance()->GetSubsystem<UReconManagerSubsystem>();

	PlayerBlipImage = NewObject<UImage>(this);
	UCanvasPanelSlot* PlayerBlipSlot = BlipContainer->AddChildToCanvas(PlayerBlipImage);
	if (MinimapManager)
	{
		PlayerBlipSize = MinimapManager->GetBlipSizeForID(PlayerBlipID);
		UMaterialInterface* PlayerSprite = MinimapManager->GetSpriteForID(PlayerBlipID);
		if (PlayerSprite)
			PlayerBlipImage->SetBrushFromMaterial(PlayerSprite);
	}
	PlayerBlipSlot->SetSize(FVector2D(PlayerBlipSize));
	PlayerBlipSlot->SetZOrder(MinimapManager ? MinimapManager->GetBlipZOrderForID(PlayerBlipID) : 0);

	for (int32 i = 0; i < MaxBlips; i++)
	{
		UImage* BlipImage = NewObject<UImage>(this);
		BlipContainer->AddChildToCanvas(BlipImage);
		BlipImage->SetVisibility(ESlateVisibility::Collapsed);
		BlipPool.Add(BlipImage);
		BlipPoolMaterials.Add(nullptr);
		BlipPoolSizes.Add(-1.0f);
		BlipPoolZOrders.Add(-1);
	}
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AActor* Pawn = GetOwningPlayerPawn();
	if (!IsValid(Pawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("MinimapWidget: No owning pawn"));
		return;
	}

	FVector PlayerPos = Pawn->GetActorLocation();

	if (!DynamicMinimapMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("MinimapWidget: DynamicMinimapMaterial is null (BaseMinimapMaterial assigned?)"));
		return;
	}

	FVector4 Input = FVector4(
		PlayerPos.X + 100000.0f,
		PlayerPos.Y + 100000.0f,
		ViewRadius,
		200000.0f
	);
	DynamicMinimapMaterial->SetVectorParameterValue("OffsetCoordinates", Input);

	UReconManagerSubsystem* MinimapManager = GetGameInstance()->GetSubsystem<UReconManagerSubsystem>();
	if (!MinimapManager) return;

	if (UCacheManagerSubsystem* CacheManager = GetGameInstance()->GetSubsystem<UCacheManagerSubsystem>())
		CacheManager->UpdateCacheHighlights(PlayerPos, ViewRadius, MinimapManager->bCacheResonance);

	const FVector2D MinimapSize = MinimapImage->GetCachedGeometry().GetLocalSize();

	// Player blip — always centered, rotated by yaw
	const FVector2D MinimapCenter = MinimapSize * 0.5f - PlayerBlipSize * 0.5f;
	if (UCanvasPanelSlot* PlayerBlipSlot = Cast<UCanvasPanelSlot>(PlayerBlipImage->Slot))
		PlayerBlipSlot->SetPosition(FVector2D(FMath::RoundToFloat(MinimapCenter.X), FMath::RoundToFloat(MinimapCenter.Y)));

	FWidgetTransform PlayerTransform;
	PlayerTransform.Angle = Pawn->GetActorRotation().Yaw;
	PlayerBlipImage->SetRenderTransform(PlayerTransform);

	MinimapManager->GetVisibleBlips(PlayerPos, ViewRadius, VisibleBlips);

	for (int32 i = 0; i < BlipPool.Num(); i++)
	{
		if (i < VisibleBlips.Num() && VisibleBlips[i].Sprite)
		{
			UImage* BlipImage = BlipPool[i];
			UCanvasPanelSlot* BlipSlot = Cast<UCanvasPanelSlot>(BlipImage->Slot);

			if (VisibleBlips[i].Sprite != BlipPoolMaterials[i])
			{
				BlipImage->SetBrushFromMaterial(VisibleBlips[i].Sprite);
				BlipPoolMaterials[i] = VisibleBlips[i].Sprite;
			}
			if (VisibleBlips[i].BlipSize != BlipPoolSizes[i])
			{
				BlipSlot->SetSize(FVector2D(VisibleBlips[i].BlipSize));
				BlipPoolSizes[i] = VisibleBlips[i].BlipSize;
			}
			if (VisibleBlips[i].ZOrder != BlipPoolZOrders[i])
			{
				BlipSlot->SetZOrder(VisibleBlips[i].ZOrder);
				BlipPoolZOrders[i] = VisibleBlips[i].ZOrder;
			}
			const FVector2D RawPos = VisibleBlips[i].MapUV * MinimapSize - VisibleBlips[i].BlipSize * 0.5f;
			BlipSlot->SetPosition(FVector2D(FMath::RoundToFloat(RawPos.X), FMath::RoundToFloat(RawPos.Y)));
			BlipImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			BlipPool[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
