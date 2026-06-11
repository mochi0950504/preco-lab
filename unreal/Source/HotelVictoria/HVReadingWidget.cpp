// 閱讀文件 UI 基類實作：翻頁、鍵盤操作（E/Space 下一頁、Esc 關閉）、關閉廣播。
#include "HVReadingWidget.h"

#include "Input/Events.h"
#include "InputCoreTypes.h"

void UHVReadingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// 必須可聚焦才能收到 NativeOnKeyDown。
	SetIsFocusable(true);
}

void UHVReadingWidget::SetupReading(const FText& InTitle, const TArray<FText>& InPages)
{
	Title = InTitle;
	Pages = InPages;
	if (Pages.Num() == 0)
	{
		Pages.Add(FText::GetEmpty());
	}
	PageIndex = 0;
	RefreshContent();
}

void UHVReadingWidget::NextPage()
{
	if (PageIndex + 1 >= Pages.Num())
	{
		CloseReading();
		return;
	}
	++PageIndex;
	RefreshContent();
}

void UHVReadingWidget::PrevPage()
{
	if (PageIndex > 0)
	{
		--PageIndex;
		RefreshContent();
	}
}

void UHVReadingWidget::CloseReading()
{
	OnReadingClosed.Broadcast();
}

void UHVReadingWidget::RefreshContent()
{
	const FText PageText = Pages.IsValidIndex(PageIndex) ? Pages[PageIndex] : FText::GetEmpty();
	OnReadingContentChanged(Title, PageText, PageIndex, Pages.Num());
}

FReply UHVReadingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Escape)
	{
		CloseReading();
		return FReply::Handled();
	}
	if (Key == EKeys::E || Key == EKeys::SpaceBar || Key == EKeys::Enter)
	{
		NextPage();
		return FReply::Handled();
	}
	if (Key == EKeys::Q || Key == EKeys::Left)
	{
		PrevPage();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
