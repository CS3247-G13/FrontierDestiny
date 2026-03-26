#pragma once

#include "TowerData.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ModeComponent.h"
#include "BuilderComponent.generated.h"

class UTowerData;
class UEconomySubsystem;
class UQuestSubsystem;
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

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FGhostTowerPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<ATowerActor*> Actors;
};

USTRUCT(BlueprintType)
struct FRONTIERDESTINY_API FTowerDisplay
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	bool Present = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	FTowerData Data;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTowerSelectionChange, FTowerDisplay, SelectedTower, FTowerDisplay, NextTower1, FTowerDisplay, NextTower2, FTowerDisplay, NextTower3);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTowerBuildingNotification, FString, Notificatoin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHoverTowerStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHoverTowerStop);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTowerSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTowerDeselected);

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
	void InitializeGhostPool();

	// ====== BUIDLING VISUALS ====== //
public:
	UPROPERTY(VisibleAnywhere, Category = "Setup")
	EGridVisualState GridVisualState = EGridVisualState::Completed;

protected:
	UPROPERTY(EditAnywhere, Category = "Setup")
	UMaterialInterface* GridVisualMaterial;
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<UCurveFloat> FadeCurve;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float FadeInSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float FadeOutSpeed = 1.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Debug")
	float NormalizedGridVisualProgress = 0.f;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	UMaterialInstanceDynamic* GridVisualMID;
	UPROPERTY()
	TObjectPtr<UPostProcessComponent> PostProcessComponent;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TObjectPtr<AGridActor> ClosestGridActor;

private:
	void EnterGridVisual();
	void ExitGridVisual();
	void UpdateGridVisualState(float DeltaSeconds);
	void InitializePostProcessMaterial();
	void CheckForClosestGridActor();
	void UpdatePostProcessComponentProgress();
	void UpdatePostProcessComponentOffset();
	void UpdatePostProcessComponentOccupancyBitmask();

	// ====== BUILDING MODE ====== //
protected:
	virtual void ActivateMode() override;
	virtual void DeactivateMode() override;


	// ====== TOWER BUILDING ====== //
protected:
	UPROPERTY(EditAnywhere, Category = "Setup")
	TObjectPtr<USoundBase> BuildSound;
	/*
	Array of Tower Data that the player can use*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TArray<FName> AvailableTowers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TObjectPtr<UInputAction> BuildTowerAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TObjectPtr<UInputAction> SelectTowerAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TObjectPtr<UInputAction> DeselectTowerAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	TObjectPtr<UInputAction> RotateTowerAction;

private:
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FIntPoint LockedGridLocationStart;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	FIntPoint LockedGridLocationEnd;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	bool bIsLocked;
	UPROPERTY(EditAnywhere, Category = "Setup")
	int32 MaxTowers = 10;

	UFUNCTION()
	void OnBuildTowerActionStart(const FInputActionValue& Value);
	UFUNCTION()
	void OnBuildTowerActionEnd(const FInputActionValue& Value);
	UFUNCTION()
	void OnSelectTowerAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnDeselectTowerAction(const FInputActionValue& Value);
	UFUNCTION()
	void OnRotateTowerAction(const FInputActionValue& Value);

	/*
	Changes the current selected tower to the provided tower data,
	updating the ghost structure. If no tower data is provided,
	it clears the selection.*/
	void ChangeTowerSelection(TOptional<FName> NewTower);

	void TryBuildTowers();

	/*
	Updates the ghost structure's location based on set grid index*/
	void UpdateGhostStructure();
	/*
	Updates the ghost structure's rotation based on player camera*/
	void UpdateGhostStructureRotation();

	/*
	Performs the raycast to find the grid and location to place the ghost tower. This should
	only be called by pawn if it has a camera, else it will return false.*/
	bool TryRaycastToGrid(FHitResult& Hit);

	/*
	Performs the raycast to check for tower to destroy*/
	bool TryRaycastToTower(FHitResult& Hit, ATowerActor*& HitTowerActor);

	/* Displays a single ghost tower */
	void DisplayGhostTower(const FIntPoint& PivotPointIndex, const FTowerData& TowerData, int GhostTowerIndex);

	UPROPERTY()
	TMap<FName, FGhostTowerPool> GhostTowerPool;

	UPROPERTY()
	TArray<ATowerActor*> ActiveGhostTowers;

	// Rotate the tower
	void RotateTower(bool Clockwise);

	ERotation AddedBuildingRotation;
	ERotation BuildingRotationRelativeToBuilder;

	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TOptional<FName> SelectedTower;
	UPROPERTY(VisibleAnywhere, Category = "Debug")
	TArray<int32> SelectedPath;

	UPROPERTY(BlueprintAssignable)
	FOnTowerSelectionChange OnTowerSelectionChange;
	UPROPERTY(BlueprintAssignable)
	FOnTowerBuildingNotification OnTowerBuildingNotification;

	// ====== DELETE TOWER ====== //
public:
	UPROPERTY(BlueprintAssignable)
	FOnHoverTowerStart OnHoverTowerStart;

	UPROPERTY(BlueprintAssignable)
	FOnHoverTowerStop OnHoverTowerStop;

	UPROPERTY(BlueprintAssignable)
	FOnTowerSelected OnTowerSelected;

	UPROPERTY(BlueprintAssignable)
	FOnTowerDeselected OnTowerDeselected;

private:
	UPROPERTY()
	TObjectPtr<ATowerActor> HoveredTower;
	UFUNCTION()
	void DeleteHoveredTower();
	UFUNCTION()
	void UpdateHoveredTower(ATowerActor* NewHoveredTower);

	void GetSelectedTowerData(FTowerData& TowerData);

	FName LoadedTower;
	FTowerData CachedTowerData;
	// ====== Configurations ====== //
public:
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
	TObjectPtr<UEconomySubsystem> EconomyComponent;
	TObjectPtr<UQuestSubsystem> QuestComponent;
};