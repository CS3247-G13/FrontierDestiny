#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ModeComponent.h"
#include "BuilderComponent.generated.h"

class UTowerData;
class UEconomyComponent;
class ATowerActor;
class AGridActor;

class UPostProcessComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

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
class FRONTIERDESTINY_API UBuilderComponent : public UModeComponent
{
	GENERATED_BODY()
	
	// STANDARD
public:
	
	UBuilderComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void TickWhenActive() override;

	virtual void SetupInput(UInputComponent* Input) override;
	void InitializeReferences();

	// ====== BUIDLING VISUALS ====== //
public:
	UPROPERTY(VisibleAnywhere, Category = "Grid Visual")
	EGridVisualState GridVisualState = EGridVisualState::Completed;

protected:
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
	UPROPERTY()
	TObjectPtr<UPostProcessComponent> PostProcessComponent;

private:
	void EnterGridVisual();
	void ExitGridVisual();
	void UpdateGridVisualState(float DeltaSeconds);
	void InitializePostProcessMaterial();
	void UpdatePostProcessComponent();

	// ====== BUILDING MODE ====== //
protected:
	virtual void ActivateMode() override;
	virtual void DeactivateMode() override;


	// ====== TOWER BUILDING ====== //
protected:
	/*
	Array of Tower Data that the player can use*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower")
	TArray<TObjectPtr<UTowerData>> AvailableTowers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> BuildTowerAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SelectTowerAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> RotateTowerAction;

private:
	UFUNCTION()
	void OnBuildTowerAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnSelectTowerAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnRotateTowerAction(const FInputActionValue& Value);

	/*
	Changes the current selected tower to the provided tower data,
	updating the ghost structure. If no tower data is provided,
	it clears the selection.*/
	void ChangeTowerSelection(UTowerData* NewTowerData);

	/*
	Tries to build the selected tower at the provided location based on the
	ghost structure, returns false if the location is invalid*/
	bool TryBuildTower();

	/*
	Destroy and rebuild the ghost structure with new blueprint.*/
	void UpdateGhostStructureBlueprint();

	/*
	Update the color of the ghost structure.*/
	void UpdateGhostStructureValid();

	/*
	Check if tower can be placed*/
	bool CheckTowerCanBePlaced();

	/*
	Updates the ghost structure's location based on set grid index*/
	void UpdateGhostStructureLocation();
	/*
	Updates the ghost structure's rotation based on player camera*/
	void UpdateGhostStructureRotation();

	/*
	Sets the grid that is currently being used for building*/
	void SetGrid(AGridActor* NewGrid);

	/*
	Performs the raycast to find the grid and location to place the ghost tower. This should
	only be called by pawn if it has a camera, else it will return false.*/
	bool TryPerformRaycast(FHitResult& Hit);

	// Rotate the tower
	void RotateTower(bool Clockwise);

	ERotation AddedBuildingRotation;
	ERotation BuildingRotationRelativeToBuilder;

	UPROPERTY()
	TObjectPtr<ATowerActor> GhostTowerActor;
	UPROPERTY()
	TObjectPtr<UTowerData> SelectedTowerData;
	UPROPERTY()
	TObjectPtr<AGridActor> GridActor;

	UPROPERTY()
	bool bCanPlaceTower = false;
	UPROPERTY()
	FIntPoint CurrentGridLocationIndex;

	// ====== Configurations ====== //
public:
	/*
	Range that the builder can build from*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	float BuildRange = 10000.f;

	/*
	Whether the builder component is a raycasting builder.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool bIsRaycastBuilder = true;

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