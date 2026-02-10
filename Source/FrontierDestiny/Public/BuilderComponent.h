#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuilderComponent.generated.h"

class UTowerData;
class UEconomyComponent;
class ATowerActor;
class AGridActor;

UENUM(BlueprintType)
enum class ERotation : uint8
{
	ROT_0   UMETA(DisplayName = "0 Degrees"),
	ROT_90  UMETA(DisplayName = "90 Degrees"),
	ROT_180 UMETA(DisplayName = "180 Degrees"),
	ROT_270 UMETA(DisplayName = "270 Degrees")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRONTIERDESTINY_API UBuilderComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	
	UBuilderComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	/*
	Whether the pawn is in building mode. This determines if this component
	is active*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bIsBuildingModeActive = false;

	/*
	Range that the builder can build from*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	float buildRange = 10000.f;

	/*
	Whether the builder component is a raycasting builder.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bIsRaycastBuilder = true;

	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void RotateTower(bool Clockwise);
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
};