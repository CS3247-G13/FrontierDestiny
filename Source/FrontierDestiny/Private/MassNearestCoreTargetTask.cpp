#include "MassNearestCoreTargetTask.h"
#include "MassAIBehaviorTypes.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "CoreManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassNearestCoreTargetTask)

bool FMassNearestCoreTargetTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(AgentRadiusHandle);
	return true;
}

EStateTreeRunStatus FMassNearestCoreTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const FMassStateTreeExecutionContext& MassContext = static_cast<const FMassStateTreeExecutionContext&>(Context);
	UWorld* World = MassContext.GetEntityManager().GetWorld();
	if (!World) return EStateTreeRunStatus::Failed;

	UCoreManagerSubsystem* CoreManager = World->GetSubsystem<UCoreManagerSubsystem>();
	if (!CoreManager) return EStateTreeRunStatus::Failed;

	const FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);
	const FVector EntityLocation = TransformFragment.GetTransform().GetLocation();

	const FVector CoreLocation = CoreManager->GetNearestActiveCoreLocation(EntityLocation);

	InstanceData.TargetLocation.EndOfPathPosition = CoreLocation;
	InstanceData.TargetLocation.EndOfPathIntent = EMassMovementAction::Move;

	return EStateTreeRunStatus::Running;
}
