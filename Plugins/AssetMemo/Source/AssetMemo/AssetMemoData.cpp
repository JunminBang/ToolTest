#include "AssetMemoData.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

FString FAssetMemoStorage::GetFilePath()
{
	return FPaths::ProjectSavedDir() / TEXT("AssetMemo.json");
}

TArray<TSharedPtr<FAssetMemoFolder>> FAssetMemoStorage::Load()
{
	TArray<TSharedPtr<FAssetMemoFolder>> Result;

	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *GetFilePath()))
	{
		return Result;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		return Result;
	}

	for (const TSharedPtr<FJsonValue>& FolderVal : JsonArray)
	{
		TSharedPtr<FJsonObject> FolderObj = FolderVal->AsObject();
		if (!FolderObj.IsValid()) continue;

		TSharedPtr<FAssetMemoFolder> Folder = MakeShared<FAssetMemoFolder>();
		FolderObj->TryGetStringField(TEXT("Name"), Folder->Name);
		FolderObj->TryGetBoolField(TEXT("Expanded"), Folder->bExpanded);

		const TArray<TSharedPtr<FJsonValue>>* EntriesArray;
		if (FolderObj->TryGetArrayField(TEXT("Entries"), EntriesArray))
		{
			for (const TSharedPtr<FJsonValue>& EntryVal : *EntriesArray)
			{
				TSharedPtr<FJsonObject> EntryObj = EntryVal->AsObject();
				if (!EntryObj.IsValid()) continue;

				TSharedPtr<FAssetMemoEntry> Entry = MakeShared<FAssetMemoEntry>();
				EntryObj->TryGetStringField(TEXT("ObjectPath"), Entry->ObjectPath);
				EntryObj->TryGetStringField(TEXT("AssetName"),  Entry->AssetName);
				EntryObj->TryGetStringField(TEXT("AssetClass"), Entry->AssetClass);
				EntryObj->TryGetStringField(TEXT("Status"),     Entry->Status);
				EntryObj->TryGetStringField(TEXT("Memo"),       Entry->Memo);
				Folder->Entries.Add(Entry);
			}
		}

		Result.Add(Folder);
	}

	return Result;
}

void FAssetMemoStorage::Save(const TArray<TSharedPtr<FAssetMemoFolder>>& Folders)
{
	TArray<TSharedPtr<FJsonValue>> JsonArray;

	for (const TSharedPtr<FAssetMemoFolder>& Folder : Folders)
	{
		TSharedPtr<FJsonObject> FolderObj = MakeShared<FJsonObject>();
		FolderObj->SetStringField(TEXT("Name"),     Folder->Name);
		FolderObj->SetBoolField  (TEXT("Expanded"), Folder->bExpanded);

		TArray<TSharedPtr<FJsonValue>> EntriesJson;
		for (const TSharedPtr<FAssetMemoEntry>& Entry : Folder->Entries)
		{
			TSharedPtr<FJsonObject> EntryObj = MakeShared<FJsonObject>();
			EntryObj->SetStringField(TEXT("ObjectPath"), Entry->ObjectPath);
			EntryObj->SetStringField(TEXT("AssetName"),  Entry->AssetName);
			EntryObj->SetStringField(TEXT("AssetClass"), Entry->AssetClass);
			EntryObj->SetStringField(TEXT("Status"),     Entry->Status);
			EntryObj->SetStringField(TEXT("Memo"),       Entry->Memo);
			EntriesJson.Add(MakeShared<FJsonValueObject>(EntryObj));
		}
		FolderObj->SetArrayField(TEXT("Entries"), EntriesJson);

		JsonArray.Add(MakeShared<FJsonValueObject>(FolderObj));
	}

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(JsonArray, Writer);

	FFileHelper::SaveStringToFile(Output, *GetFilePath());
}
