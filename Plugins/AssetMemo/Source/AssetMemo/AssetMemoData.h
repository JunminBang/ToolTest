#pragma once

#include "CoreMinimal.h"

struct FAssetMemoEntry
{
	FString ObjectPath;
	FString AssetName;
	FString AssetClass;
	FString Status = TEXT("미작업");
	FString Memo;
};

struct FAssetMemoFolder
{
	FString Name;
	bool    bExpanded = true;
	TArray<TSharedPtr<FAssetMemoEntry>> Entries;
};

class FAssetMemoStorage
{
public:
	static TArray<TSharedPtr<FAssetMemoFolder>> Load();
	static void Save(const TArray<TSharedPtr<FAssetMemoFolder>>& Folders);

private:
	static FString GetFilePath();
};
