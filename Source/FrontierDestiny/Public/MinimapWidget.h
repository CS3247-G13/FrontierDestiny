// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "BlipData.h"
#include "MinimapWidget.generated.h"

class UCanvasPanel;

UCLASS()
class FRONTIERDESTINY_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> MinimapImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> BlipContainer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<UMaterialInterface> BaseMinimapMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<UMaterialInstanceDynamic> DynamicMinimapMaterial;

	/** ID used to look up the player blip sprite in the blip data table. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	FName PlayerBlipID = "player";

	/** Maximum number of blips that can be displayed simultaneously. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	int32 MaxBlips = 100;

	/** World-unit radius of the minimap view. Matches the material's radius parameter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	float ViewRadius = 10000.0f;

	/** World-unit radius within which enemy blips are shown. Enemies beyond ViewRadius but within this range are clamped to the minimap edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	float EnemyBlipRadius = 20000.0f;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TArray<TObjectPtr<UImage>> BlipPool;
	TArray<TObjectPtr<UMaterialInterface>> BlipPoolMaterials;
	TArray<float> BlipPoolSizes;
	TArray<int32> BlipPoolZOrders;

	TObjectPtr<UImage> PlayerBlipImage;
	float PlayerBlipSize = 24.0f;
	TArray<FBlipRenderEntry> VisibleBlips;
};
