#include "AssetMemoModule.h"
#include "SAssetMemoWidget.h"

#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Docking/TabManager.h"

#define LOCTEXT_NAMESPACE "FAssetMemoModule"

static const FName AssetMemoTabName(TEXT("AssetMemo"));

void FAssetMemoModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		AssetMemoTabName,
		FOnSpawnTab::CreateRaw(this, &FAssetMemoModule::SpawnTab)
	)
	.SetDisplayName(LOCTEXT("TabTitle", "Asset Memo"))
	.SetTooltipText(LOCTEXT("TabTooltip", "에셋에 메모와 상태를 기록합니다"));
}

void FAssetMemoModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AssetMemoTabName);
}

TSharedRef<SDockTab> FAssetMemoModule::SpawnTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SAssetMemoWidget)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetMemoModule, AssetMemo)
