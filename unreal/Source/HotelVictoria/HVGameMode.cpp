// 遊戲主流程 GameMode 實作：狀態切換、證據登記、閱讀 Widget 生命週期、1013 解鎖。
#include "HVGameMode.h"

#include "HotelVictoria.h"
#include "HVEvidenceActor.h"
#include "HVGhostDirector.h"
#include "HVPlayerCharacter.h"
#include "HVReadingWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AHVGameMode::AHVGameMode()
{
	// 未建立藍圖子類前也能直接遊玩的安全預設。
	DefaultPawnClass = AHVPlayerCharacter::StaticClass();
}

void AHVGameMode::BeginPlay()
{
	Super::BeginPlay();
	SetGameState(InitialState);
}

void AHVGameMode::SetGameState(EHVGameState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;
	UE_LOG(LogHotelVictoria, Log, TEXT("GameState -> %s"), *UEnum::GetValueAsString(NewState));

	// 只有自由探索狀態允許女鬼導演排程事件。
	if (UHVGhostDirector* Director = GetWorld()->GetSubsystem<UHVGhostDirector>())
	{
		Director->SetEnabled(NewState == EHVGameState::Play);
	}

	OnGameStateChanged.Broadcast(NewState);
}

void AHVGameMode::StartFinale()
{
	if (CurrentState == EHVGameState::Finale)
	{
		return;
	}
	SetGameState(EHVGameState::Finale);
}

bool AHVGameMode::RegisterEvidence(FName EvidenceId)
{
	if (EvidenceId.IsNone() || CollectedEvidenceIds.Contains(EvidenceId))
	{
		return false;
	}

	CollectedEvidenceIds.Add(EvidenceId);
	UE_LOG(LogHotelVictoria, Log, TEXT("收集證據 %s（%d / %d）"),
		*EvidenceId.ToString(), CollectedEvidenceIds.Num(), RequiredEvidenceCount);

	OnEvidenceChanged.Broadcast(CollectedEvidenceIds.Num(), RequiredEvidenceCount);

	// 證據越多，女鬼出沒越頻繁、越大膽。
	if (UHVGhostDirector* Director = GetWorld()->GetSubsystem<UHVGhostDirector>())
	{
		Director->NotifyEvidenceCount(CollectedEvidenceIds.Num());
	}

	if (!bRoom1013Unlocked && CollectedEvidenceIds.Num() >= RequiredEvidenceCount)
	{
		UnlockRoom1013();
	}
	return true;
}

void AHVGameMode::UnlockRoom1013()
{
	bRoom1013Unlocked = true;
	UE_LOG(LogHotelVictoria, Log, TEXT("五份證據齊全——本不存在的 1013 房浮現。"));
	// 關卡藍圖監聽此委派：讓 10F 牆面退開、1013 房門淡入、播放低鳴音效。
	OnRoom1013Unlocked.Broadcast();
}

void AHVGameMode::BeginEvidenceReading(AHVEvidenceActor* Evidence)
{
	if (!Evidence || CurrentState == EHVGameState::Reading)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	if (!ReadingWidget && ReadingWidgetClass && PC)
	{
		ReadingWidget = CreateWidget<UHVReadingWidget>(PC, ReadingWidgetClass);
	}

	if (!ReadingWidget)
	{
		// 尚未指定閱讀 Widget 類別時，直接視為讀畢收取，流程不至於卡死。
		UE_LOG(LogHotelVictoria, Warning, TEXT("ReadingWidgetClass 未設定，直接收取證據 %s。"),
			*Evidence->GetEvidenceId().ToString());
		Evidence->MarkCollected();
		return;
	}

	PendingEvidence = Evidence;
	ReadingWidget->OnReadingClosed.AddUniqueDynamic(this, &AHVGameMode::HandleReadingClosed);
	ReadingWidget->SetupReading(Evidence->GetEvidenceTitle(), Evidence->GetPages());

	if (!ReadingWidget->IsInViewport())
	{
		ReadingWidget->AddToViewport(/*ZOrder=*/10);
	}

	if (PC)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ReadingWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}

	BroadcastInteractHint(false, FText::GetEmpty());
	SetGameState(EHVGameState::Reading);
}

void AHVGameMode::HandleReadingClosed()
{
	if (ReadingWidget)
	{
		ReadingWidget->RemoveFromParent();
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	// 讀畢即收取：證據 Actor 會回頭呼叫 RegisterEvidence 並自我隱藏。
	if (AHVEvidenceActor* Evidence = PendingEvidence.Get())
	{
		Evidence->MarkCollected();
	}
	PendingEvidence = nullptr;

	SetGameState(EHVGameState::Play);
}

void AHVGameMode::BroadcastInteractHint(bool bVisible, const FText& HintText)
{
	OnInteractHintChanged.Broadcast(bVisible, HintText);
}
