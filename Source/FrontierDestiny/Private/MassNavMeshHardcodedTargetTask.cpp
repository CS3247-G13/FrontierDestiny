
#include "MassNavMeshHardcodedTargetTask.h"
#include "MassAIBehaviorTypes.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "MassStateTreeExecutionContext.h"

#include "StateTreeLinker.h"
#include <MassDebugger.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassNavMeshHardcodedTargetTask)

bool FMassNavMeshHardcodedTargetTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(AgentRadiusHandle);
	return true;
}

EStateTreeRunStatus FMassNavMeshHardcodedTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// Hardcoded target location (0,0,0)
	//const FVector HardcodedTarget = FVector::ZeroVector;
	//const FVector HardcodedTarget = FVector(-3000, -2000, 0);

	// Fill TargetLocation for NavMeshPathFollow task
	InstanceData.TargetLocation.EndOfPathPosition = InstanceData.HardcodedTarget;
	InstanceData.TargetLocation.EndOfPathIntent = EMassMovementAction::Move;

#if WITH_MASSGAMEPLAY_DEBUG
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	if (UE::Mass::Debug::IsDebuggingEntity(MassContext.GetEntity()))
	{
		MASSBEHAVIOR_LOG(Log, TEXT("Hardcoded target set at %s"), *(InstanceData.HardcodedTarget).ToString());
	}
#endif

	return EStateTreeRunStatus::Running;
}