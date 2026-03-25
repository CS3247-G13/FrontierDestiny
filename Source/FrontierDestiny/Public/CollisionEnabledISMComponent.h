// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "CollisionEnabledISMComponent.generated.h"

/**
 * 
 */
UCLASS()
class FRONTIERDESTINY_API UCollisionEnabledISMComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()
	
	virtual void OnRegister() override;
};
