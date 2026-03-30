#include "AnimToolsBlueprintLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"

bool UAnimToolsBlueprintLibrary::HasBone(USkeletalMesh* SkeletalMesh, FName BoneName)
{
	if (!IsValid(SkeletalMesh))
	{
		return false;
	}
	return SkeletalMesh->GetRefSkeleton().FindBoneIndex(BoneName) != INDEX_NONE;
}

void UAnimToolsBlueprintLibrary::SetupDuplicatedAnimBlueprint(UAnimBlueprint* AnimBlueprint, USkeletalMesh* PreviewMesh)
{
	if (!IsValid(AnimBlueprint))
	{
		return;
	}

	AnimBlueprint->bIsTemplate = false;

	if (IsValid(PreviewMesh))
	{
		AnimBlueprint->SetPreviewMesh(PreviewMesh);
	}
}
