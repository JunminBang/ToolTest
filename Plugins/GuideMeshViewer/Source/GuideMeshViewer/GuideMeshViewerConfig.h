#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/StaticMesh.h"
#include "GuideMeshViewerConfig.generated.h"

// ChangedProp: 변경된 최상위 프로퍼티 이름 (NAME_None이면 전체 갱신)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGuideMeshConfigChanged, class UGuideMeshViewerConfig*, FName);

UCLASS()
class UGuideMeshViewerConfig : public UObject
{
	GENERATED_BODY()

public:
	// 가이드 메시 표시 여부
	UPROPERTY(EditAnywhere, Category = "Guide Mesh", meta = (DisplayName = "Visible"))
	bool bVisible = true;

	// 뷰포트에 표시할 가이드 스태틱 메시
	UPROPERTY(EditAnywhere, Category = "Guide Mesh", meta = (DisplayName = "Static Mesh"))
	TSoftObjectPtr<UStaticMesh> GuideMesh;

	// 가이드 메시의 위치/회전/스케일
	UPROPERTY(EditAnywhere, Category = "Guide Mesh")
	FTransform Transform = FTransform::Identity;

	// 머티리얼 "Color" 벡터 파라미터로 전달
	UPROPERTY(EditAnywhere, Category = "Material")
	FLinearColor Color = FLinearColor::White;

	// 머티리얼 "Opacity" 스칼라 파라미터로 전달
	UPROPERTY(EditAnywhere, Category = "Material", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 1.0f;

	FOnGuideMeshConfigChanged OnConfigChanged;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};
