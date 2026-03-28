#include "SAssetMemoWidget.h"
#include "AssetMemoData.h"

#include "DragAndDrop/AssetDragDropOp.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "SAssetMemoWidget"

static const FName ColName(TEXT("Name"));
static const FName ColType(TEXT("Type"));
static const FName ColStatus(TEXT("Status"));
static const FName ColMemo(TEXT("Memo"));
static const FName ColAction(TEXT("Action"));

static const TArray<TSharedPtr<FString>>& GetStatusOptions()
{
	static TArray<TSharedPtr<FString>> Options = {
		MakeShared<FString>(TEXT("미작업")),
		MakeShared<FString>(TEXT("진행중")),
		MakeShared<FString>(TEXT("완료")),
	};
	return Options;
}

// ─────────────────────────────────────────────────────────────────────────────
// 트리 행 위젯
// ─────────────────────────────────────────────────────────────────────────────

class SAssetMemoTreeRow : public SMultiColumnTableRow<TSharedPtr<FMemoTreeItem>>
{
public:
	SLATE_BEGIN_ARGS(SAssetMemoTreeRow) {}
		SLATE_ARGUMENT(TSharedPtr<FMemoTreeItem>, Item)
		SLATE_ARGUMENT(TWeakPtr<SAssetMemoWidget>, OwnerWidget)
		SLATE_ARGUMENT(bool, bIsRenaming)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
	{
		Item        = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;
		bIsRenaming = InArgs._bIsRenaming;

		SMultiColumnTableRow::Construct(FSuperRowType::FArguments()
			.Padding(Item->IsFolder() ? FMargin(0, 2) : FMargin(0)),
			InOwnerTable);

		if (bIsRenaming && NameEditBox.IsValid())
		{
			RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda(
				[EditBox = TWeakPtr<SEditableTextBox>(NameEditBox)](double, float) -> EActiveTimerReturnType
				{
					if (TSharedPtr<SEditableTextBox> Box = EditBox.Pin())
					{
						FSlateApplication::Get().SetKeyboardFocus(Box, EFocusCause::SetDirectly);
						Box->SelectAllText();
					}
					return EActiveTimerReturnType::Stop;
				}
			));
		}
	}

	// 폴더 행은 드롭 수신
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		if (!Item->IsFolder()) return SMultiColumnTableRow::OnDrop(MyGeometry, DragDropEvent);

		TSharedPtr<FAssetDragDropOp> DragOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
		if (DragOp.IsValid())
		{
			if (TSharedPtr<SAssetMemoWidget> W = OwnerWidget.Pin())
			{
				W->AddAssetsToFolder(DragOp->GetAssets(), Item->Folder);
			}
			return FReply::Handled();
		}
		return SMultiColumnTableRow::OnDrop(MyGeometry, DragDropEvent);
	}

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		if (!Item->IsFolder()) return;
		TSharedPtr<FAssetDragDropOp> DragOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
		if (DragOp.IsValid())
		{
			DragOp->SetToolTip(
				FText::Format(LOCTEXT("DropToFolder", "'{0}'에 추가"), FText::FromString(Item->Folder->Name)),
				FAppStyle::GetBrush("Graph.ConnectorFeedback.OK"));
		}
	}

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override
	{
		TSharedPtr<FAssetDragDropOp> DragOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
		if (DragOp.IsValid())
		{
			DragOp->ResetToDefaultToolTip();
		}
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		// ── 폴더 행 ──────────────────────────────────────────────────────────
		if (Item->IsFolder())
		{
			if (ColumnName == ColName)
			{
				TWeakPtr<SAssetMemoWidget> WeakW = OwnerWidget;
				TSharedPtr<FMemoTreeItem>  ItemRef = Item;

				return SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("NoBorder"))
					.Padding(FMargin(2, 2))
					.Cursor(EMouseCursor::Hand)
					[
						SNew(SHorizontalBox)
						// 폴더 이름 (평소: 텍스트, F2: 편집)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						.Padding(2, 0)
						[
							BuildFolderNameWidget(ItemRef, WeakW)
						]
						// 항목 수
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0)
						[
							SNew(STextBlock)
							.Text_Lambda([ItemRef]()
							{
								return FText::Format(
									LOCTEXT("EntryCount", "({0})"),
									FText::AsNumber(ItemRef->Children.Num()));
							})
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						// 폴더 삭제
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
							.ToolTipText(LOCTEXT("DeleteFolder", "폴더 삭제 (항목도 함께 삭제)"))
							.OnClicked_Lambda([ItemRef, WeakW]() -> FReply
							{
								if (TSharedPtr<SAssetMemoWidget> W = WeakW.Pin())
								{
									W->RemoveFolder(ItemRef->Folder);
								}
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("✕")))
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							]
						]
					];
			}
			return SNullWidget::NullWidget;
		}

		// ── 에셋 행 ──────────────────────────────────────────────────────────
		if (ColumnName == ColName)
		{
			return SNew(SBox)
				.Padding(FMargin(2, 1))
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ToolTipText(FText::FromString(Item->Entry->ObjectPath))
					.OnClicked_Lambda([ObjectPath = Item->Entry->ObjectPath]() -> FReply
					{
						IAssetRegistry& AssetRegistry =
							FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
						FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(ObjectPath));
						if (AssetData.IsValid())
						{
							FContentBrowserModule& CBModule =
								FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
							CBModule.Get().SyncBrowserToAssets(TArray<FAssetData>{ AssetData });
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(Item->Entry->AssetName))
					]
				];
		}

		if (ColumnName == ColType)
		{
			return SNew(SBox)
				.Padding(FMargin(6, 3))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->Entry->AssetClass))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				];
		}

		if (ColumnName == ColStatus)
		{
			TSharedPtr<FString> Current = GetStatusOptions()[0];
			for (const TSharedPtr<FString>& Opt : GetStatusOptions())
			{
				if (*Opt == Item->Entry->Status) { Current = Opt; break; }
			}

			TWeakPtr<SAssetMemoWidget> WeakW = OwnerWidget;
			TSharedPtr<FAssetMemoEntry> EntryRef = Item->Entry;

			return SNew(SBox)
				.Padding(FMargin(2, 1))
				.VAlign(VAlign_Center)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&GetStatusOptions())
					.InitiallySelectedItem(Current)
					.OnSelectionChanged_Lambda([EntryRef, WeakW](TSharedPtr<FString> Val, ESelectInfo::Type)
					{
						if (Val.IsValid())
						{
							EntryRef->Status = *Val;
							if (TSharedPtr<SAssetMemoWidget> W = WeakW.Pin()) W->SaveData();
						}
					})
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
					{
						return SNew(STextBlock)
							.Text(FText::FromString(Item.IsValid() ? *Item : FString()))
							.Margin(FMargin(4, 2));
					})
					[
						SNew(STextBlock)
						.Text_Lambda([EntryRef]() { return FText::FromString(EntryRef->Status); })
					]
				];
		}

		if (ColumnName == ColMemo)
		{
			TWeakPtr<SAssetMemoWidget> WeakW = OwnerWidget;
			TSharedPtr<FAssetMemoEntry> EntryRef = Item->Entry;

			return SNew(SBox)
				.Padding(FMargin(2, 1))
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(Item->Entry->Memo))
					.HintText(LOCTEXT("MemoHint", "메모 입력..."))
					.OnTextCommitted_Lambda([EntryRef, WeakW](const FText& NewText, ETextCommit::Type)
					{
						EntryRef->Memo = NewText.ToString();
						if (TSharedPtr<SAssetMemoWidget> W = WeakW.Pin()) W->SaveData();
					})
				];
		}

		if (ColumnName == ColAction)
		{
			TWeakPtr<SAssetMemoWidget> WeakW   = OwnerWidget;
			TSharedPtr<FAssetMemoEntry> EntryRef = Item->Entry;
			TWeakPtr<FMemoTreeItem> WeakOwner   = Item->OwnerFolderItem;

			return SNew(SBox)
				.Padding(FMargin(2, 1))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ToolTipText(LOCTEXT("RemoveTip", "목록에서 제거"))
					.OnClicked_Lambda([EntryRef, WeakOwner, WeakW]() -> FReply
					{
						TSharedPtr<FMemoTreeItem> OwnerItem = WeakOwner.Pin();
						if (OwnerItem.IsValid())
						{
							if (TSharedPtr<SAssetMemoWidget> W = WeakW.Pin())
							{
								W->RemoveEntry(EntryRef, OwnerItem->Folder);
							}
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("✕")))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				];
		}

		return SNullWidget::NullWidget;
	}

	TSharedRef<SWidget> BuildFolderNameWidget(
		TSharedPtr<FMemoTreeItem> ItemRef,
		TWeakPtr<SAssetMemoWidget> WeakW)
	{
		if (bIsRenaming)
		{
			return SAssignNew(NameEditBox, SEditableTextBox)
				.Text(FText::FromString(ItemRef->Folder->Name))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.SelectAllTextWhenFocused(true)
				.OnTextCommitted_Lambda([WeakW](const FText& NewText, ETextCommit::Type)
				{
					if (TSharedPtr<SAssetMemoWidget> W = WeakW.Pin())
					{
						W->EndRename(NewText);
					}
				});
		}
		return SNew(STextBlock)
			.Text_Lambda([ItemRef]() { return FText::FromString(ItemRef->Folder->Name); })
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10));
	}

private:
	TSharedPtr<FMemoTreeItem>    Item;
	TWeakPtr<SAssetMemoWidget>   OwnerWidget;
	bool                         bIsRenaming = false;
	TSharedPtr<SEditableTextBox> NameEditBox;
};

// ─────────────────────────────────────────────────────────────────────────────
// 메인 위젯
// ─────────────────────────────────────────────────────────────────────────────

void SAssetMemoWidget::Construct(const FArguments& InArgs)
{
	Folders = FAssetMemoStorage::Load();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(6)
		[
			SNew(SVerticalBox)

			// 툴바
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("NewFolder", "+ 새 폴더"))
					.ToolTipText(LOCTEXT("NewFolderTip", "새 폴더 추가"))
					.OnClicked_Lambda([this]() -> FReply
					{
						AddFolder();
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(8, 0, 0, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DropHint", "폴더 위 또는 빈 공간에 에셋을 드래그&드롭"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]

			// 트리뷰
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(TreeView, STreeView<TSharedPtr<FMemoTreeItem>>)
				.TreeItemsSource(&RootItems)
				.OnGenerateRow(this, &SAssetMemoWidget::GenerateRow)
				.OnGetChildren(this, &SAssetMemoWidget::GetTreeChildren)
				.OnMouseButtonClick_Lambda([this](TSharedPtr<FMemoTreeItem> ClickedItem)
				{
					if (ClickedItem.IsValid() && ClickedItem->IsFolder() && TreeView.IsValid())
					{
						ActiveFolderItem = ClickedItem;
						const bool bExpanded = TreeView->IsItemExpanded(ClickedItem);
						TreeView->SetItemExpansion(ClickedItem, !bExpanded);
					}
				})
				.OnExpansionChanged_Lambda([this](TSharedPtr<FMemoTreeItem> Item, bool bExpanded)
				{
					if (Item->IsFolder())
					{
						Item->Folder->bExpanded = bExpanded;
						SaveData();
					}
				})
				.SelectionMode(ESelectionMode::None)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(ColName)
					  .DefaultLabel(LOCTEXT("ColName", "에셋 이름"))
					  .FillWidth(3.0f)
					+ SHeaderRow::Column(ColType)
					  .DefaultLabel(LOCTEXT("ColType", "타입"))
					  .FillWidth(2.0f)
					+ SHeaderRow::Column(ColStatus)
					  .DefaultLabel(LOCTEXT("ColStatus", "상태"))
					  .FillWidth(2.0f)
					+ SHeaderRow::Column(ColMemo)
					  .DefaultLabel(LOCTEXT("ColMemo", "메모"))
					  .FillWidth(5.0f)
					+ SHeaderRow::Column(ColAction)
					  .DefaultLabel(FText::GetEmpty())
					  .FixedWidth(36.0f)
				)
			]
		]
	];

	RebuildTree();
}

TSharedRef<ITableRow> SAssetMemoWidget::GenerateRow(
	TSharedPtr<FMemoTreeItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bIsRenaming = Item->IsFolder() && Item->Folder == RenamingFolder;
	return SNew(SAssetMemoTreeRow, OwnerTable)
		.Item(Item)
		.OwnerWidget(SharedThis(this))
		.bIsRenaming(bIsRenaming);
}

void SAssetMemoWidget::GetTreeChildren(
	TSharedPtr<FMemoTreeItem> Item,
	TArray<TSharedPtr<FMemoTreeItem>>& OutChildren)
{
	OutChildren = Item->Children;
}

void SAssetMemoWidget::RebuildTree()
{
	RootItems.Empty();

	for (TSharedPtr<FAssetMemoFolder>& Folder : Folders)
	{
		TSharedPtr<FMemoTreeItem> FolderItem = MakeShared<FMemoTreeItem>();
		FolderItem->Folder = Folder;

		for (TSharedPtr<FAssetMemoEntry>& Entry : Folder->Entries)
		{
			TSharedPtr<FMemoTreeItem> EntryItem = MakeShared<FMemoTreeItem>();
			EntryItem->Entry           = Entry;
			EntryItem->OwnerFolderItem = FolderItem;
			FolderItem->Children.Add(EntryItem);
		}

		RootItems.Add(FolderItem);
	}

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();

		for (TSharedPtr<FMemoTreeItem>& Item : RootItems)
		{
			TreeView->SetItemExpansion(Item, Item->Folder->bExpanded);
		}
	}
}

void SAssetMemoWidget::AddFolder(const FString& Name)
{
	TSharedPtr<FAssetMemoFolder> NewFolder = MakeShared<FAssetMemoFolder>();
	NewFolder->Name      = Name;
	NewFolder->bExpanded = true;
	Folders.Add(NewFolder);

	SaveData();
	RebuildTree();
}

TSharedPtr<FAssetMemoFolder> SAssetMemoWidget::GetDefaultFolder()
{
	if (Folders.Num() == 0)
	{
		AddFolder(TEXT("미분류"));
	}
	return Folders[0];
}

void SAssetMemoWidget::AddAssetsToFolder(
	const TArray<FAssetData>& Assets,
	TSharedPtr<FAssetMemoFolder> TargetFolder)
{
	bool bAdded = false;

	for (const FAssetData& Asset : Assets)
	{
		const FString Path = Asset.GetSoftObjectPath().ToString();

		// 전체 폴더에서 중복 검사
		bool bExists = false;
		for (const TSharedPtr<FAssetMemoFolder>& F : Folders)
		{
			if (F->Entries.ContainsByPredicate(
				[&Path](const TSharedPtr<FAssetMemoEntry>& E) { return E->ObjectPath == Path; }))
			{
				bExists = true;
				break;
			}
		}
		if (bExists) continue;

		TSharedPtr<FAssetMemoEntry> Entry = MakeShared<FAssetMemoEntry>();
		Entry->ObjectPath = Path;
		Entry->AssetName  = Asset.AssetName.ToString();
		Entry->AssetClass = Asset.AssetClassPath.GetAssetName().ToString();
		TargetFolder->Entries.Add(Entry);
		bAdded = true;
	}

	if (bAdded)
	{
		TargetFolder->bExpanded = true;
		SaveData();
		RebuildTree();
	}
}

void SAssetMemoWidget::RemoveEntry(
	TSharedPtr<FAssetMemoEntry> Entry,
	TSharedPtr<FAssetMemoFolder> OwnerFolder)
{
	OwnerFolder->Entries.Remove(Entry);
	SaveData();
	RebuildTree();
}

void SAssetMemoWidget::RemoveFolder(TSharedPtr<FAssetMemoFolder> Folder)
{
	if (ActiveFolderItem.IsValid() && ActiveFolderItem->Folder == Folder)
	{
		ActiveFolderItem = nullptr;
	}
	if (RenamingFolder == Folder)
	{
		RenamingFolder = nullptr;
	}
	Folders.Remove(Folder);
	SaveData();
	RebuildTree();
}

void SAssetMemoWidget::SaveData()
{
	FAssetMemoStorage::Save(Folders);
}

void SAssetMemoWidget::BeginRename()
{
	if (!ActiveFolderItem.IsValid()) return;
	RenamingFolder = ActiveFolderItem->Folder;
	RebuildTree();
}

void SAssetMemoWidget::EndRename(const FText& NewText)
{
	if (RenamingFolder.IsValid())
	{
		RenamingFolder->Name = NewText.ToString();
	}
	RenamingFolder = nullptr;
	SaveData();
	RebuildTree();
}

FReply SAssetMemoWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::F2)
	{
		BeginRename();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

bool SAssetMemoWidget::SupportsKeyboardFocus() const
{
	return true;
}

FReply SAssetMemoWidget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FAssetDragDropOp> DragOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (DragOp.IsValid())
	{
		AddAssetsToFolder(DragOp->GetAssets(), GetDefaultFolder());
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SAssetMemoWidget::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FAssetDragDropOp> DragOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (DragOp.IsValid())
	{
		DragOp->SetToolTip(
			LOCTEXT("DropToDefault", "기본 폴더에 추가"),
			FAppStyle::GetBrush("Graph.ConnectorFeedback.OK"));
	}
}

void SAssetMemoWidget::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FAssetDragDropOp> DragOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (DragOp.IsValid())
	{
		DragOp->ResetToDefaultToolTip();
	}
}

#undef LOCTEXT_NAMESPACE
