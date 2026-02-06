#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuilderComponent.generated.h"

class UTowerData;
class ATowerActor;
class AGridActor;

class UInputAction;
class FInputActionValue;

UENUM(BlueprintType)
enum class ERotation : uint8
{
	ROT_0   UMETA(DisplayName = "0 Degrees"),
	ROT_90  UMETA(DisplayName = "90 Degrees"),
	ROT_180 UMETA(DisplayName = "180 Degrees"),
	ROT_270 UMETA(DisplayName = "270 Degrees")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), Within = Pawn)
class FRONTIERDESTINY_API UBuilderComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	
	UBuilderComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*
	Changes the current selected tower to the provided tower data,
	updating the ghost structure. If no tower data is provided,
	it clears the selection.*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void ChangeTowerSelection(TOptional<UTowerData> NewTowerData);

	/*
	Builds the selected tower at the provided location based on the
	ghost structure*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void BuildTower();

	/*
	Checks if there has been a change in the ghost structure. If the tower data
	(blueprint) changed, then destroy and rebuild the ghost structure. Else,
	update it accordingly*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void UpdateGhostStructureVisuals();

	/*
	Updates the ghost structure's location and rotation based on world location*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void UpdateGhostStructureLocationAndRotationByWorldLocation(FVector LookAtLocation);

	/*
	Updates the ghost structure's location and rotation based on grid index*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void UpdateGhostStructureLocationAndRotationByGridIndex(FIntPoint NewGridIndex);

	/*
	Sets the grid that is currently being used for building*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	void SetGrid(AGridActor* NewGrid);

	/*
	Performs the raycast to find the grid and location to place the ghost tower. This should
	only be called by pawn if it has a camera*/
	UFUNCTION(BlueprintCallable, Category = "Tower Building")
	bool UpdateGhostStructureLocationByRaycast();

	/*
	The Input action that maps to building the tower*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> BuildInputAction;
	/*
	The Input action that maps to rotating the tower*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> RotateInputAction;

	/*
	Whether the pawn is in building mode. This determines if this component
	is active*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bIsBuildingModeActive = false;
private:
	ERotation addedBuildingRotation;
	ERotation buildingRotationWrtBuilder;

	UPROPERTY()
	TObjectPtr<ATowerActor> GhostTowerActor;
	UPROPERTY()
	TObjectPtr<UTowerData> SelectedTowerData;
	UPROPERTY()
	TObjectPtr<AGridActor> GridActor;

	bool bCanPlaceBuilding = false;
	FIntPoint CurrentGridLocationIndex;

	/*
	Get rotation relative to pawn using direction from building to player*/
	ERotation GetBuildingRotationWithRegardsToBuilder();
	
	void OnRotateInput(const FInputActionValue& Value);

	void OnBuildInput(const FInputActionValue& Value);

	// ========= Helpers ==========
	
	ERotation GetBuildingRotation() const
	{
		return static_cast<ERotation>(
			((static_cast<uint8>(addedBuildingRotation) 
				+ static_cast<uint8>(buildingRotationWrtBuilder)) % 4)
		);
	}

	FRotator GetBuildingRotator() const
	{
		return FRotator{ 0.f, 90.f * static_cast<uint8>(GetBuildingRotation()), 0.f };
	}


	
};