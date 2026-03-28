#pragma once

#include "Modules/ModuleManager.h"

class FAssetMemoModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);
};
