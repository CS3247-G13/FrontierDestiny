// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BlipData.generated.h"

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FBlipData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Blip")
	FName ID = NAME_None;

	/** Material used to render this blip icon on the minimap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Blip")
	TSoftObjectPtr<UMaterialInterface> Sprite = nullptr;

	/** Size of this blip in slate units. Blips are always square. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Blip")
	float BlipSize = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Blip")
	int32 ZOrder = 0;
};

/** Produced each tick by MinimapManagerSubsystem — one entry per visible enemy. */
USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FBlipRenderEntry
{
	GENERATED_BODY()

	/** UV position on the full map texture [0, 1]. Used to place the blip on the render target. */
	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Blip")
	FVector2D MapUV = FVector2D::ZeroVector;

	/** Pre-resolved sprite material, ready to pass to UCanvas. */
	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Blip")
	TObjectPtr<UMaterialInterface> Sprite = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Blip")
	float BlipSize = 24.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Minimap|Blip")
	int32 ZOrder = 0;
};
