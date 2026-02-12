// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	ATowerActor();

	// Called when the actor is spawned or properties are changed in the editor
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	virtual void Tick(float DeltaSeconds) override;
protected:
	UFUNCTION()
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Tower")
	void OnTowerActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tower")
	void OnTowerTick(float DeltaTime);

	/** The detection radius for finding enemies */
	UPROPERTY()
	TObjectPtr<USphereComponent> RangeComponent;

	/** List of actors currently inside the range */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
	TArray<AActor*> OverlappingTargets;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Setup")
	bool bIsGhost = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Setup")
	bool bIsValidGhost = true;

	/** The range of the tower in world units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Setup")
	float TowerRange = 500.0f;

	/** The specific class of actor this tower is allowed to target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TSubclassOf<AActor> TargetClassFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Setup")
	TObjectPtr<UTowerData> TowerInfo;

	UFUNCTION(BlueprintCallable, Category = "Tower Functions|Ghost")
	void SetGhostValidity(bool bNewIsValid);

	/** Logic to select a target from OverlappingTargets */
	UFUNCTION(Category = "Tower Functions")
	virtual void SelectTarget();

	/** Time interval between overlap checks (Optimization) */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float OverlapCheckInterval;

	UFUNCTION()
	TArray<UTowerData*> GetUpgrades();

	UPROPERTY()
	TObjectPtr<AGridActor> GridActor;
	UPROPERTY()
	FIntPoint CornerGridIndex;

private:
	UFUNCTION()
	void OnRangeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRangeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void CheckAllOverlaps();

	void UpdateGhostMaterials();

	FTimerHandle OverlapCheckTimerHandle;

	FTimerHandle BuildTimerHandle;

	void InitializeGhostTower();

	void InitializeTower();

	void ActivateTower();

	bool bIsBuilding = true;
};