#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AssetMemoData.h"

struct FAssetData;

// 트리 노드 — 폴더 또는 에셋 항목
struct FMemoTreeItem
{
	TSharedPtr<FAssetMemoFolder> Folder;        // 폴더 노드
	TSharedPtr<FAssetMemoEntry>  Entry;         // 에셋 노드
	TWeakPtr<FMemoTreeItem>      OwnerFolderItem;
	TArray<TSharedPtr<FMemoTreeItem>> Children;

	bool IsFolder() const { return Folder.IsValid(); }
};

class SAssetMemoWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetMemoWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;

	// 행 위젯에서 호출
	void AddAssetsToFolder(const TArray<FAssetData>& Assets, TSharedPtr<FAssetMemoFolder> TargetFolder);
	void RemoveEntry(TSharedPtr<FAssetMemoEntry> Entry, TSharedPtr<FAssetMemoFolder> OwnerFolder);
	void RemoveFolder(TSharedPtr<FAssetMemoFolder> Folder);
	void SaveData();
	void EndRename(const FText& NewText);

private:
	void AddFolder(const FString& Name = TEXT("새 폴더"));
	void RebuildTree();
	void BeginRename();
	TSharedPtr<FAssetMemoFolder> GetDefaultFolder();

	TSharedRef<ITableRow> GenerateRow(TSharedPtr<FMemoTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void GetTreeChildren(TSharedPtr<FMemoTreeItem> Item, TArray<TSharedPtr<FMemoTreeItem>>& OutChildren);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override;

	TArray<TSharedPtr<FAssetMemoFolder>>   Folders;
	TArray<TSharedPtr<FMemoTreeItem>>      RootItems;
	TSharedPtr<STreeView<TSharedPtr<FMemoTreeItem>>> TreeView;
	TSharedPtr<FMemoTreeItem>    ActiveFolderItem;
	TSharedPtr<FAssetMemoFolder> RenamingFolder;
};
