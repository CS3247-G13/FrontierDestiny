#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuilderComponent.generated.h"

class UTowerData;
class UEconomyComponent;
class ATowerActor;
class AGridActor;

class UPostProcessComponent;
class UCameraComponent;

UENUM(BlueprintType)
enum class ERotation : uint8
{
	ROT_0   UMETA(DisplayName = "0 Degrees"),
	ROT_90  UMETA(DisplayName = "90 Degrees"),
	ROT_180 UMETA(DisplayName = "180 Degrees"),
	ROT_270 UMETA(DisplayName = "270 Degrees")
};

UENUM(BlueprintType)
enum class EGridVisualState : uint8
{
	FadingIn	UMETA(DisplayName = "Fading in"),
	Presenting	UMETA(DisplayName = "Presenting"),
	FadingOut	UMETA(DisplayName = "Fading out"),
	Completed	UMETA(DisplayName = "Completed"),
	Invalid		UMETA(DisplayName = "Invalid")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRONTIERDESTINY_API UBuilderComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	
	UBuilderComponent();

	virtual void BeginPlay() override;
	void InitializeReferences();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ====== BUIDLING VISUALS ====== //
	UPROPERTY(VisibleAnywhere, Category = "Grid Visual")
	EGridVisualState GridVisualState = EGridVisualState::Completed;

	UFUNCTION(BlueprintCallable, Category = "Grid Visual")
	void EnterGridVisual();
	UFUNCTION(BlueprintCallable, Category = "Grid Visual")
	void ExitGridVisual();
	UFUNCTION()
	void UpdateGridVisualState(float DeltaSeconds);
	UFUNCTION()
	void InitializePostProcessMaterial();
	UFUNCTION()
	void UpdatePostProcessComponent();

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> PostProcessComponent;
	UPROPERTY(EditAnywhere, Category = "Grid Visual")
	UMaterialInterface* GridVisualMaterial;
	UPROPERTY(VisibleAnywhere, Category = "Grid Visual")
	UMaterialInstanceDynamic* GridVisualMID;
	UPROPERTY(EditAnywhere, Category = "Grid Visual")
	TObjectPtr<UCurveFloat> FadeCurve;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grid Visual")
	float NormalizedGridVisualProgress = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Visual")
	float FadeInSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Visual")
	float FadeOutSpeed = 1.f;

	// ====== BUILDING MODE ====== //

	/*
	Sets the player to be in building mode, activating this component*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void ActivateBuildingMode();

	/*
	Sets the player to be not in building mode, deactivating this component*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void DeactivateBuildingMode();

	/*
	Toggles the player's buiding mode, activating it if it is deactivated and vice versa*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void ToggleBuildingMode();

	/*
	Whether the pawn is in building mode. This determines if this component
	is active*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bIsBuildingModeActive = false;

	// ====== TOWER BUILDING ====== //

	/*
	Changes the current selected tower to the provided tower data,
	updating the ghost structure. If no tower data is provided,
	it clears the selection.*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void ChangeTowerSelection(UTowerData* NewTowerData);

	/*
	Tries to build the selected tower at the provided location based on the
	ghost structure, returns false if the location is invalid*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	bool TryBuildTower();

	/*
	Destroy and rebuild the ghost structure with new blueprint.*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void UpdateGhostStructureBlueprint();

	/*
	Update the color of the ghost structure.*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void UpdateGhostStructureValid();

	/*
	Check if tower can be placed*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	bool CheckTowerCanBePlaced();

	/*
	Updates the ghost structure's location based on set grid index*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void UpdateGhostStructureLocation();
	/*
	Updates the ghost structure's rotation based on player camera*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void UpdateGhostStructureRotation();

	/*
	Sets the grid that is currently being used for building*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void SetGrid(AGridActor* NewGrid);

	/*
	Performs the raycast to find the grid and location to place the ghost tower. This should
	only be called by pawn if it has a camera, else it will return false.*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	bool TryPerformRaycast(FHitResult& Hit);

	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void RotateTower(bool Clockwise);

	// ====== Configurations ====== //

	/*
	Range that the builder can build from*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	float BuildRange = 10000.f;

	/*
	Whether the builder component is a raycasting builder.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bIsRaycastBuilder = true;

private:
	ERotation AddedBuildingRotation;
	ERotation BuildingRotationRelativeToBuilder;

	UPROPERTY(VisibleInstanceOnly, Category = "Debug")
	TObjectPtr<ATowerActor> GhostTowerActor;
	UPROPERTY(VisibleInstanceOnly, Category = "Debug")
	TObjectPtr<UTowerData> SelectedTowerData;
	UPROPERTY(VisibleInstanceOnly, Category = "Debug")
	TObjectPtr<AGridActor> GridActor;

	UPROPERTY(VisibleInstanceOnly, Category = "Debug")
	bool bCanPlaceTower = false;
	UPROPERTY(VisibleInstanceOnly, Category = "Debug")
	FIntPoint CurrentGridLocationIndex;
	UPROPERTY(VisibleInstanceOnly, Category = "Debug")
	bool bDebugMode = false;

	// ========= Helpers ==========
	
	ERotation GetBuildingRotation() const
	{
		return static_cast<ERotation>(
			((static_cast<uint8>(AddedBuildingRotation) 
				+ static_cast<uint8>(BuildingRotationRelativeToBuilder)) % 4)
		);
	}

	FRotator GetBuildingRotator() const
	{
		return FRotator{ 0.f, 90.f * static_cast<uint8>(GetBuildingRotation()), 0.f };
	}
	
	UPROPERTY(VisibleInstanceOnly, Category = "References")
	TObjectPtr<UEconomyComponent> EconomyComponent;

	UPROPERTY(VisibleInstanceOnly, Category = "References")
	TObjectPtr<UCameraComponent> Camera;
};