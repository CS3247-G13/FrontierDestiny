// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TowerData.h"
#include "CoreMinimal.h"
#include "MassEnemyTarget.h"
#include "GameFramework/Actor.h"
#include "TowerActor.generated.h"

class UTowerData;
class USphereComponent;
class UMaterialInterface;
class AGridActor;

UCLASS()
class FRONTIERDESTINY_API ATowerActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Setup")
	FName TowerID;

	ATowerActor();

	// Called when the actor is spawned or properties are changed in the editor
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable)
	virtual void DestroyTower();

	UFUNCTION(BlueprintCallable)
	void SetValidOverlayMaterial();

	UFUNCTION(BlueprintCallable)
	void SetInvalidOverlayMaterial();
	UFUNCTION(BlueprintCallable)
	void ClearOverlayMaterial();

	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	FTowerData TowerData;

	UFUNCTION(BlueprintCallable)
	void UpdateStats();
	UFUNCTION(BlueprintPure)
	float GetStats(FGameplayTag Tag);

protected:
	UFUNCTION()
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Tower")
	void OnTowerActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tower")
	void OnTowerTick(float DeltaTime);

	virtual void ActivateTower();

	/** The detection radius for finding enemies */
	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Debug")
	TObjectPtr<USphereComponent> RangeComponent;

	bool bTowerIsInactive = true;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Setup")
	bool bIsGhost = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Setup")
	bool bIsValidGhost = true;

	UFUNCTION(BlueprintCallable, Category = "Tower Functions|Ghost")
	void SetGhostValidity(bool bNewIsValid);

	UPROPERTY()
	TObjectPtr<AGridActor> GridActor;
	UPROPERTY()
	FIntPoint CornerGridIndex;
	UPROPERTY()
	FRotator GridRelativeRotation;

private:
	FTimerHandle BuildTimerHandle;

	void InitializeGhostTower();
	void InitializeTower();
	void UpdateGhostMaterials();
};
