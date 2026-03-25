
#pragma once

#include "MassNavigationTypes.h"
#include "MassStateTreeTypes.h"
#include "MassNavMeshHardcodedTargetTask.generated.h"

struct FAgentRadiusFragment;
struct FStateTreeExecutionContext;
struct FTransformFragment;

USTRUCT(BlueprintType)
struct FMassNavMeshHardcodedTargetTaskInstanceData
{
	GENERATED_BODY()

	/** Target location to go to (editable in BP) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Parameters)
	FVector HardcodedTarget = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation TargetLocation;
};

/**
 * Sets TargetLocation to a hardcoded location (0,0,0) for NavMesh path following.
 */
USTRUCT(meta = (DisplayName = "NavMesh Hardcoded Target"))
struct FMassNavMeshHardcodedTargetTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassNavMeshHardcodedTargetTaskInstanceData;

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FAgentRadiusFragment> AgentRadiusHandle;
};