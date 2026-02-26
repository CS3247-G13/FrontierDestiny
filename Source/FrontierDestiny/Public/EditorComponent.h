// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EditorComponent.generated.h"

// Forward declarations to keep the header clean
class ATowerActor;
class ATowerDemoPlayerController;
class UEconomyComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTowerSelected, FName, TowerID);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRONTIERDESTINY_API UEditorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UEditorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void ActivateEditorMode();
	UFUNCTION()
	void DeactivateEditorMode();

	// Locks the selection to the hovered object
	UFUNCTION(BlueprintCallable, Category = "Tower Editor")
	void LockSelectedTower();
	// Unlocks the selection
	UFUNCTION(BlueprintCallable, Category = "Tower Editor")
	void UnlockSelectedTower();
	// For controller to call when the lock/unlock button is pressed
	UFUNCTION(BlueprintCallable, Category = "Tower Editor")
	void ToggleLock();

	void UpdateSelectedTower(ATowerActor* NewTower);

	/** Deletes the currently selected tower and potentially refunds some gold */
	UFUNCTION(BlueprintCallable, Category = "Tower Editor")
	void DeleteSelectedTower();

	/** Performs a raycast from the camera to find potential tower interactions */
	UFUNCTION(BlueprintCallable, Category = "Tower Editor")
	bool TryPerformRaycast(FHitResult& Hit);

	UFUNCTION()
	void UpdateOutlineColor(FLinearColor NewColor);

	UPROPERTY(EditAnywhere, Category = "Effects")
	UMaterialInterface* OutlineMaterialBase;
	UPROPERTY(VisibleAnywhere, Category = "Effects")
	UMaterialInstanceDynamic* OutlineMID;

	// UI will update the tower cards
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnTowerSelected OnTowerSelected;

private:

	/** Reference to the currently selected tower actor */
	UPROPERTY(VisibleAnywhere, Category = "Tower Editor")
	TObjectPtr<ATowerActor> SelectedTower;
	UPROPERTY()
	TObjectPtr<UEconomyComponent> EconomyComponent;

	/** Range that the editor can interact with towers from */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor Settings", meta = (AllowPrivateAccess = "true"))
	float InteractionRange = 10000.f;

	/** Helper to find and cache the Player Controller and Economy */
	void InitializeComponentReferences();
	void InitializeOutlineMaterial();

	UPROPERTY(EditAnywhere, Category = "Tower Editor")
	bool bIsComponentActive;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	bool bIsSelectionLocked = false;

	UFUNCTION()
	void SelectAndHighlightTower(ATowerActor* ActorToHighlight);
};