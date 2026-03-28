#pragma once

#include "Modules/ModuleManager.h"
#include "UObject/GCObject.h"
#include "Templates/SharedPointer.h"

class IPersonaPreviewScene;
class UStaticMeshComponent;
class UGuideMeshViewerConfig;
class AActor;

// 프리뷰 씬 하나당 생성되는 상태 묶음
struct FGuideMeshSceneData
{
	TWeakPtr<IPersonaPreviewScene>      PreviewSceneWeak;
	TObjectPtr<UGuideMeshViewerConfig>  Config     = nullptr;
	TObjectPtr<AActor>                  GuideActor = nullptr;
	TObjectPtr<UStaticMeshComponent>    Component  = nullptr;
	FDelegateHandle                     ConfigChangedHandle;
	FName                               TabId;
};

class FGuideMeshViewerModule : public IModuleInterface, public FGCObject
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// FGCObject — Config/Actor/Component를 GC로부터 보호
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FGuideMeshViewerModule"); }

private:
	void OnPreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& PreviewScene);

	// 탭 등록: Persona 툴킷 호스트가 준비된 후 다음 틱에 실행
	void TryRegisterTab(TWeakPtr<FGuideMeshSceneData> WeakData);

	// 설정 변경 시 호출
	void ApplyConfig(FGuideMeshSceneData& Data, UGuideMeshViewerConfig* Config, FName ChangedProp);

	// 메시가 바뀌었을 때 컴포넌트 전체 재생성
	void RebuildComponent(FGuideMeshSceneData& Data);

	// 색상/투명도만 MID에 적용
	void UpdateMaterialParams(FGuideMeshSceneData& Data);

	void CleanupSceneData(FGuideMeshSceneData& Data);

	FDelegateHandle                         PreviewSceneCreatedHandle;
	TArray<TSharedPtr<FGuideMeshSceneData>> SceneDataArray;
};
