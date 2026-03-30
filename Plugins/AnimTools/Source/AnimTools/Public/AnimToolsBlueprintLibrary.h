#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimToolsBlueprintLibrary.generated.h"

class UAnimBlueprint;

UCLASS()
class ANIMTOOLS_API UAnimToolsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** SkeletalMesh의 Reference Skeleton에서 본 이름을 확인합니다. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Anim Tools|Skeleton",
		meta = (DisplayName = "Has Bone", Keywords = "bone skeleton check find"))
	static bool HasBone(USkeletalMesh* SkeletalMesh, FName BoneName);

	/**
	 * 복제된 AnimBlueprint의 템플릿 플래그를 해제하고 프리뷰 메시를 지정합니다.
	 * Python에서 직접 접근 불가한 프로퍼티를 C++로 노출합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Anim Tools|Blueprint",
		meta = (DisplayName = "Setup Duplicated Anim Blueprint"))
	static void SetupDuplicatedAnimBlueprint(UAnimBlueprint* AnimBlueprint, USkeletalMesh* PreviewMesh);
};
