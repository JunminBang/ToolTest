#include "GuideMeshViewerConfig.h"

void UGuideMeshViewerConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// MemberProperty: 사용자가 직접 편집한 최상위 프로퍼티 (Transform 서브필드 등도 포함)
	const FName TopPropName = PropertyChangedEvent.MemberProperty
		? PropertyChangedEvent.MemberProperty->GetFName()
		: NAME_None;

	OnConfigChanged.Broadcast(this, TopPropName);
}
