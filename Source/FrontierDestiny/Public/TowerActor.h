// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TowerData.h"
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
	UPROPERTY()
	FTowerData CachedTowerData;

	UFUNCTION(BlueprintCallable)
	virtual void UpdateStats(FName ID);

	UFUNCTION(BlueprintCallable)
	int32 GetDamage();
	UFUNCTION(BlueprintCallable)
	float GetCooldown();
	UFUNCTION(BlueprintCallable)
	int32 GetHealth();
	UFUNCTION(BlueprintCallable)
	int32 GetRange();

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

	/** The specific class of actor this tower is allowed to target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TSubclassOf<AActor> TargetClassFilter;


	UFUNCTION(BlueprintCallable, Category = "Tower Functions|Ghost")
	void SetGhostValidity(bool bNewIsValid);

	/** Logic to select a target from OverlappingTargets */
	UFUNCTION(Category = "Tower Functions")
	virtual void SelectTarget();

	/** Time interval between overlap checks (Optimization) */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float OverlapCheckInterval;

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

	bool bTowerIsInactive = true;
};