// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ResourceAmount.generated.h"

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FResourceAmount
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Cryxite = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Nullsteel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Gravstone = 0;

	bool IsZero() const
	{
		return Cryxite == 0 && Nullsteel == 0 && Gravstone == 0;
	}

	// Returns true if every component of this is >= the corresponding component of Other
	bool CanAfford(const FResourceAmount& Cost) const
	{
		return Cryxite >= Cost.Cryxite && Nullsteel >= Cost.Nullsteel && Gravstone >= Cost.Gravstone;
	}

	FResourceAmount operator+(const FResourceAmount& Other) const
	{
		return { Cryxite + Other.Cryxite, Nullsteel + Other.Nullsteel, Gravstone + Other.Gravstone };
	}

	FResourceAmount operator-(const FResourceAmount& Other) const
	{
		return { Cryxite - Other.Cryxite, Nullsteel - Other.Nullsteel, Gravstone - Other.Gravstone };
	}

	FResourceAmount& operator+=(const FResourceAmount& Other)
	{
		Cryxite += Other.Cryxite; Nullsteel += Other.Nullsteel; Gravstone += Other.Gravstone;
		return *this;
	}

	FResourceAmount& operator-=(const FResourceAmount& Other)
	{
		Cryxite -= Other.Cryxite; Nullsteel -= Other.Nullsteel; Gravstone -= Other.Gravstone;
		return *this;
	}
};
