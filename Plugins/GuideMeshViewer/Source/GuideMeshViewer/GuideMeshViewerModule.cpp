#include "GuideMeshViewerModule.h"
#include "GuideMeshViewerConfig.h"

#include "PersonaModule.h"
#include "IPersonaPreviewScene.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Containers/Ticker.h"

#define LOCTEXT_NAMESPACE "FGuideMeshViewerModule"

static int32 GGuideMeshTabCounter = 0;

// ─────────────────────────────────────────────────────────────────────────────
// 모듈 생명주기
// ─────────────────────────────────────────────────────────────────────────────

void FGuideMeshViewerModule::StartupModule()
{
	if (!FModuleManager::Get().IsModuleLoaded("Persona"))
	{
		return;
	}

	FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
	PreviewSceneCreatedHandle = PersonaModule.OnPreviewSceneCreated().AddRaw(
		this, &FGuideMeshViewerModule::OnPreviewSceneCreated);
}

void FGuideMeshViewerModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("Persona"))
	{
		FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");
		PersonaModule.OnPreviewSceneCreated().Remove(PreviewSceneCreatedHandle);
	}

	for (TSharedPtr<FGuideMeshSceneData>& Data : SceneDataArray)
	{
		if (Data.IsValid())
		{
			CleanupSceneData(*Data);
		}
	}
	SceneDataArray.Empty();
}

void FGuideMeshViewerModule::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (TSharedPtr<FGuideMeshSceneData>& Data : SceneDataArray)
	{
		if (!Data.IsValid()) continue;
		if (Data->Config)     Collector.AddReferencedObject(Data->Config);
		if (Data->GuideActor) Collector.AddReferencedObject(Data->GuideActor);
		if (Data->Component)  Collector.AddReferencedObject(Data->Component);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 프리뷰 씬 생성 콜백
// ─────────────────────────────────────────────────────────────────────────────

void FGuideMeshViewerModule::OnPreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& PreviewScene)
{
	TSharedPtr<FGuideMeshSceneData> Data = MakeShared<FGuideMeshSceneData>();
	Data->PreviewSceneWeak = PreviewScene;
	Data->Config = NewObject<UGuideMeshViewerConfig>(GetTransientPackage(), NAME_None, RF_Transient);

	SceneDataArray.Add(Data);

	// 설정 변경 콜백 (WeakPtr 캡처로 순환 참조 방지)
	TWeakPtr<FGuideMeshSceneData> WeakData(Data);
	Data->ConfigChangedHandle = Data->Config->OnConfigChanged.AddLambda(
		[this, WeakData](UGuideMeshViewerConfig* Config, FName ChangedProp)
		{
			if (TSharedPtr<FGuideMeshSceneData> D = WeakData.Pin())
			{
				ApplyConfig(*D, Config, ChangedProp);
			}
		}
	);

	// 툴킷 호스트는 OnPreviewSceneCreated 시점에 미설정일 수 있어 다음 틱으로 지연
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this, WeakData](float) -> bool
		{
			TryRegisterTab(WeakData);
			return false; // 한 번만 실행
		}),
		0.0f
	);
}

// ─────────────────────────────────────────────────────────────────────────────
// 탭 등록
// ─────────────────────────────────────────────────────────────────────────────

void FGuideMeshViewerModule::TryRegisterTab(TWeakPtr<FGuideMeshSceneData> WeakData)
{
	TSharedPtr<FGuideMeshSceneData> Data = WeakData.Pin();
	if (!Data.IsValid()) return;

	TSharedPtr<IPersonaPreviewScene> Scene = Data->PreviewSceneWeak.Pin();
	if (!Scene.IsValid()) return;

	Data->TabId = FName(*FString::Printf(TEXT("GuideMeshViewer_%d"), GGuideMeshTabCounter++));

	TWeakObjectPtr<UGuideMeshViewerConfig> WeakConfig(Data->Config);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		Data->TabId,
		FOnSpawnTab::CreateLambda([WeakConfig](const FSpawnTabArgs&) -> TSharedRef<SDockTab>
		{
			FPropertyEditorModule& PropEditor =
				FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

			FDetailsViewArgs Args;
			Args.bHideSelectionTip = true;
			Args.bAllowSearch      = false;

			TSharedRef<IDetailsView> DetailsView = PropEditor.CreateDetailView(Args);

			if (UGuideMeshViewerConfig* Config = WeakConfig.Get())
			{
				DetailsView->SetObject(Config);
			}

			return SNew(SDockTab)
				.TabRole(ETabRole::NomadTab)
				[
					DetailsView
				];
		})
	)
	.SetDisplayName(LOCTEXT("GuideMeshViewerTab", "Guide Mesh Viewer"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

	// 에디터 열릴 때 자동으로 탭 표시
	FGlobalTabmanager::Get()->TryInvokeTab(Data->TabId);
}

// ─────────────────────────────────────────────────────────────────────────────
// 설정 적용
// ─────────────────────────────────────────────────────────────────────────────

void FGuideMeshViewerModule::ApplyConfig(FGuideMeshSceneData& Data, UGuideMeshViewerConfig* Config, FName ChangedProp)
{
	static const FName VisibleProp   = GET_MEMBER_NAME_CHECKED(UGuideMeshViewerConfig, bVisible);
	static const FName MeshProp      = GET_MEMBER_NAME_CHECKED(UGuideMeshViewerConfig, GuideMesh);
	static const FName TransformProp = GET_MEMBER_NAME_CHECKED(UGuideMeshViewerConfig, Transform);
	static const FName ColorProp     = GET_MEMBER_NAME_CHECKED(UGuideMeshViewerConfig, Color);
	static const FName OpacityProp   = GET_MEMBER_NAME_CHECKED(UGuideMeshViewerConfig, Opacity);

	if (ChangedProp == MeshProp || ChangedProp == NAME_None)
	{
		RebuildComponent(Data);
	}
	else if (ChangedProp == VisibleProp)
	{
		if (Data.Component && IsValid(Data.Component))
		{
			Data.Component->SetVisibility(Config->bVisible, true);
		}
	}
	else if (ChangedProp == TransformProp)
	{
		if (Data.Component && IsValid(Data.Component))
		{
			Data.Component->SetWorldTransform(Config->Transform);
		}
	}
	else if (ChangedProp == ColorProp || ChangedProp == OpacityProp)
	{
		UpdateMaterialParams(Data);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 컴포넌트 재생성
// ─────────────────────────────────────────────────────────────────────────────

void FGuideMeshViewerModule::RebuildComponent(FGuideMeshSceneData& Data)
{
	if (Data.GuideActor && IsValid(Data.GuideActor))
	{
		Data.GuideActor->Destroy();
	}
	Data.GuideActor = nullptr;
	Data.Component  = nullptr;

	if (!Data.Config || Data.Config->GuideMesh.IsNull()) return;

	UStaticMesh* Mesh = Data.Config->GuideMesh.LoadSynchronous();
	if (!IsValid(Mesh)) return;

	TSharedPtr<IPersonaPreviewScene> Scene = Data.PreviewSceneWeak.Pin();
	UWorld* World = Scene ? Scene->GetWorld() : nullptr;
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags = RF_Transient;
	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);
	if (!Actor) return;

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Actor, NAME_None, RF_Transient);
	Comp->SetStaticMesh(Mesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(false);
	Comp->bSelectable = false;

	Actor->SetRootComponent(Comp);
	Actor->AddOwnedComponent(Comp);
	Comp->RegisterComponent();
	Comp->SetWorldTransform(Data.Config->Transform);
	Comp->SetVisibility(Data.Config->bVisible, true);

	Data.GuideActor = Actor;
	Data.Component  = Comp;

	UpdateMaterialParams(Data);
}

// ─────────────────────────────────────────────────────────────────────────────
// MID 파라미터 적용
// ─────────────────────────────────────────────────────────────────────────────

void FGuideMeshViewerModule::UpdateMaterialParams(FGuideMeshSceneData& Data)
{
	if (!Data.Component || !IsValid(Data.Component)) return;
	if (!Data.Config) return;

	const int32 NumMaterials = Data.Component->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		UMaterialInterface* CurMat = Data.Component->GetMaterial(i);
		if (!CurMat) continue;

		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(CurMat);
		if (!MID)
		{
			MID = UMaterialInstanceDynamic::Create(CurMat, Data.Component);
			Data.Component->SetMaterial(i, MID);
		}

		// 머티리얼에 "Color" 벡터 파라미터와 "Opacity" 스칼라 파라미터가 있어야 반영됨
		MID->SetVectorParameterValue(TEXT("Color"),   Data.Config->Color);
		MID->SetScalarParameterValue(TEXT("Opacity"), Data.Config->Opacity);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 정리
// ─────────────────────────────────────────────────────────────────────────────

void FGuideMeshViewerModule::CleanupSceneData(FGuideMeshSceneData& Data)
{
	if (Data.Config)
	{
		Data.Config->OnConfigChanged.Remove(Data.ConfigChangedHandle);
	}

	if (!Data.TabId.IsNone())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Data.TabId);
	}

	if (Data.GuideActor && IsValid(Data.GuideActor))
	{
		Data.GuideActor->Destroy();
	}

	Data.Config     = nullptr;
	Data.GuideActor = nullptr;
	Data.Component  = nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGuideMeshViewerModule, GuideMeshViewer)
