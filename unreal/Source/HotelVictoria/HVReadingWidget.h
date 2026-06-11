// 閱讀文件 UI 的 C++ 基類：標題/逐頁內文/關閉委派；UMG 子類負責實際排版。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HVReadingWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHVOnReadingClosed);

/**
 * 證據閱讀視窗基類。
 * GameMode 呼叫 SetupReading 後加入視口；翻到最後一頁再按下一頁（或按 Esc）即關閉並廣播 OnReadingClosed。
 * UMG 子類（WBP_Reading）覆寫 OnReadingContentChanged 把文字塞進 TextBlock。
 */
UCLASS(Abstract, Blueprintable)
class HOTELVICTORIA_API UHVReadingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 設定標題與逐頁文本並顯示第一頁。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Reading")
	void SetupReading(const FText& InTitle, const TArray<FText>& InPages);

	/** 下一頁；已是最後一頁則關閉。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Reading")
	void NextPage();

	/** 上一頁（第一頁時無作用）。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Reading")
	void PrevPage();

	/** 立即關閉並廣播 OnReadingClosed。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Reading")
	void CloseReading();

	UPROPERTY(BlueprintAssignable, Category = "HV|Reading")
	FHVOnReadingClosed OnReadingClosed;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 內容變化時通知 UMG 子類刷新（標題、本頁文字、頁碼）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HV|Reading")
	void OnReadingContentChanged(const FText& Title, const FText& PageText, int32 PageIndex, int32 PageCount);

private:
	void RefreshContent();

	FText Title;

	UPROPERTY()
	TArray<FText> Pages;

	int32 PageIndex = 0;
};
