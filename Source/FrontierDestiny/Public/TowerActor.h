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
	UFUNCTION(BlueprintCallable)
	float GetStats(FGameplayTag Tag);

protected:
	UFUNCTION()
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Tower")
	void OnTowerActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tower")
	void OnTowerTick(float DeltaTime);

	/** The detection radius for finding enemies */
	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Debug")
	TObjectPtr<USphereComponent> RangeComponent;

	/** Mass entity handles currently inside the tower's range */
	TSet<FMassEntityHandle> OverlappingTargets;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Setup")
	bool bIsGhost = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Setup")
	bool bIsValidGhost = true;

	UFUNCTION(BlueprintCallable, Category = "Tower Functions|Ghost")
	void SetGhostValidity(bool bNewIsValid);

	/** Called when a Mass entity enters the range of the tower */
	UFUNCTION(BlueprintNativeEvent, Category = "Tower Functions")
	void OnTargetEnterRange(FMassEnemyTarget Target);

	/** Called when a Mass entity leaves the range of the tower */
	UFUNCTION(BlueprintNativeEvent, Category = "Tower Functions")
	void OnTargetLeaveRange(FMassEnemyTarget Target);

	UPROPERTY()
	TObjectPtr<AGridActor> GridActor;
	UPROPERTY()
	FIntPoint CornerGridIndex;
	UPROPERTY()
	FRotator GridRelativeRotation;

	virtual void ActivateTower();

private:
	UFUNCTION()
	void OnRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnTrackedEnemyDied(FMassEntityHandle Handle);
	void CheckEnemiesInRange();
	void AddTarget(FMassEnemyTarget Target);
	void RemoveTarget(FMassEntityHandle Handle);

	void UpdateGhostMaterials();

	FTimerHandle BuildTimerHandle;
	FTimerHandle RangeCheckTimerHandle;

	void InitializeGhostTower();

	void InitializeTower();

	bool bTowerIsInactive = true;
};