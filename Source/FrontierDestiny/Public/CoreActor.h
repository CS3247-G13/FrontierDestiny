// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TargetPoint.h"
#include "CoreData.h"
#include "InteractableComponent.h"
#include "CoreActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoreHPChanged, ACoreActor*, UpdatedCoreData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoreActivated, ACoreActor*, CoreActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoreDestroyed, ACoreActor*, CoreActor);

UCLASS()
class FRONTIERDESTINY_API ACoreActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ACoreActor();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	FCoreData CoreData;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Core")
	bool bIsCoreActive = false;

	UPROPERTY(BlueprintAssignable, Category="Core")
	FOnCoreHPChanged OnCoreHPChanged;

	UPROPERTY(BlueprintAssignable, Category="Core")
	FOnCoreActivated OnCoreActivated;

	UPROPERTY(BlueprintAssignable, Category="Core")
	FOnCoreDestroyed OnCoreDestroyed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	int32 CoreIndex = 0;

	// HP regenerated per second when active and not recently damaged
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	float HPRegenRate = 15.f;

	// Seconds after taking damage before regen resumes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Core")
	float DamageGracePeriod = 20.f;

	UFUNCTION(BlueprintCallable)
	void ApplyDamage(float Damage);

	UFUNCTION(BlueprintCallable)
	void ActivateCore();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
	TObjectPtr<ATargetPoint> TeleportPoint;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector GetTeleportPoint() const;

	// Stencil 37 — visible through walls. Pass false to disable custom depth.
	UFUNCTION(BlueprintCallable)
	void SetShownThroughWalls(bool bEnable);

	// Stencil 38 — hovered highlight. Pass false to revert to shown-through-walls (37).
	UFUNCTION(BlueprintCallable)
	void SetHovered(bool bHovered);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	float LastDamageTime = -1.f;
	FTimerHandle RegenTimerHandle;

	UFUNCTION()
	void TickRegen();

};
