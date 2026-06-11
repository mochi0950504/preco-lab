// 女鬼導演實作：冷卻倒數、資料表加權抽選、12 種事件的生成/姿態/位移/消散、凝視提前消散。
#include "HVGhostDirector.h"

#include "HotelVictoria.h"
#include "HVGameMode.h"
#include "HVGhostCharacter.h"

#include "Engine/DataTable.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// 生命週期
// ---------------------------------------------------------------------------

void UHVGhostDirector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 嘗試從 DefaultGame.ini 設定的軟路徑載入資料表與女鬼類別；
	// BP_HVGameMode 也可以在 BeginPlay 用 SetEventTable / SetGhostClass 注入覆蓋。
	if (!GhostEventTablePath.IsNull())
	{
		EventTable = Cast<UDataTable>(GhostEventTablePath.TryLoad());
		if (!EventTable)
		{
			UE_LOG(LogHotelVictoria, Warning, TEXT("GhostDirector：載入事件資料表失敗：%s"),
				*GhostEventTablePath.ToString());
		}
	}

	if (!GhostClassPath.IsNull())
	{
		if (UClass* LoadedClass = Cast<UClass>(GhostClassPath.TryLoad()))
		{
			if (LoadedClass->IsChildOf(AHVGhostCharacter::StaticClass()))
			{
				GhostClass = LoadedClass;
			}
		}
	}

	if (!GhostClass)
	{
		// 沒有 BP 子類時用 C++ 佔位類，事件流程照常可驗證。
		GhostClass = AHVGhostCharacter::StaticClass();
	}

	CooldownRemaining = InitialCooldown;
}

void UHVGhostDirector::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DespawnTimerHandle);
	}
	Super::Deinitialize();
}

bool UHVGhostDirector::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UHVGhostDirector::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UHVGhostDirector, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// 排程主循環
// ---------------------------------------------------------------------------

void UHVGhostDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bEnabled)
	{
		return;
	}

	// 僅在自由探索狀態運作（雙重保險：GameMode 切狀態時也會 SetEnabled）。
	const AHVGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AHVGameMode>() : nullptr;
	if (!GameMode || GameMode->GetGameState() != EHVGameState::Play)
	{
		return;
	}

	if (ActiveGhost)
	{
		ActiveEventElapsed += DeltaTime;
		UpdateSeenCheck(DeltaTime);
		return; // 同時間只有一隻女鬼
	}

	CooldownRemaining -= DeltaTime;
	if (CooldownRemaining <= 0.f)
	{
		if (!TryTriggerScheduledEvent())
		{
			CooldownRemaining = RetryInterval;
		}
	}
}

void UHVGhostDirector::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
}

void UHVGhostDirector::NotifyEvidenceCount(int32 NewCount)
{
	EvidenceCount = FMath::Max(0, NewCount);
	UE_LOG(LogHotelVictoria, Log, TEXT("GhostDirector 強度係數 -> %.2f（證據 %d 份）"),
		GetIntensityScale(), EvidenceCount);
}

float UHVGhostDirector::GetIntensityScale() const
{
	return 1.f + EvidenceCount * IntensityPerEvidence;
}

int32 UHVGhostDirector::GetPlayerFloor() const
{
	if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		const float RelativeZ = Pawn->GetActorLocation().Z - GroundFloorZ;
		return FMath::Clamp(1 + FMath::FloorToInt(RelativeZ / FloorHeight), 1, 10);
	}
	return 1;
}

// ---------------------------------------------------------------------------
// 事件抽選
// ---------------------------------------------------------------------------

bool UHVGhostDirector::TryTriggerScheduledEvent()
{
	if (!EventTable)
	{
		return false;
	}

	const int32 PlayerFloor = GetPlayerFloor();

	TArray<FHVGhostEventRow*> AllRows;
	EventTable->GetAllRows<FHVGhostEventRow>(TEXT("HVGhostDirector"), AllRows);

	// 篩選：樓層範圍、證據門檻、權重 > 0、不立即重複上一種事件。
	TArray<FHVGhostEventRow*> Candidates;
	float TotalWeight = 0.f;
	for (FHVGhostEventRow* Row : AllRows)
	{
		if (!Row || Row->EventType == EHVGhostEventType::None || Row->Weight <= 0.f)
		{
			continue;
		}
		if (PlayerFloor < Row->MinFloor || PlayerFloor > Row->MaxFloor)
		{
			continue;
		}
		if (EvidenceCount < Row->MinEvidence)
		{
			continue;
		}
		if (Row->EventType == LastEventType && AllRows.Num() > 1)
		{
			continue;
		}
		Candidates.Add(Row);
		TotalWeight += Row->Weight;
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.f)
	{
		return false;
	}

	// 加權隨機；定位點缺失等失敗時最多再抽兩次。
	for (int32 Attempt = 0; Attempt < 3 && Candidates.Num() > 0; ++Attempt)
	{
		float Pick = FMath::FRandRange(0.f, TotalWeight);
		int32 PickedIndex = Candidates.Num() - 1;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Pick -= Candidates[Index]->Weight;
			if (Pick <= 0.f)
			{
				PickedIndex = Index;
				break;
			}
		}

		FHVGhostEventRow* Picked = Candidates[PickedIndex];
		if (TriggerEvent(*Picked, nullptr))
		{
			return true;
		}

		TotalWeight -= Picked->Weight;
		Candidates.RemoveAt(PickedIndex);
	}
	return false;
}

void UHVGhostDirector::RequestEventAtTransform(EHVGhostEventType EventType, const FTransform& SpawnTransform)
{
	// 點名事件（電梯倒掛等腳本驚嚇）不可被吞：現役女鬼直接回收。
	if (ActiveGhost)
	{
		GetWorld()->GetTimerManager().ClearTimer(DespawnTimerHandle);
		ActiveGhost->Destroy();
		ActiveGhost = nullptr;
	}

	// 從資料表找對應列取得 Duration / Cooldown；找不到時用安全預設值。
	FHVGhostEventRow Row;
	Row.EventType = EventType;
	Row.Duration = 3.f;
	Row.CooldownAfter = 40.f;

	if (EventTable)
	{
		TArray<FHVGhostEventRow*> AllRows;
		EventTable->GetAllRows<FHVGhostEventRow>(TEXT("HVGhostDirector"), AllRows);
		for (const FHVGhostEventRow* Candidate : AllRows)
		{
			if (Candidate && Candidate->EventType == EventType)
			{
				Row = *Candidate;
				break;
			}
		}
	}

	TriggerEvent(Row, &SpawnTransform);
}

bool UHVGhostDirector::TriggerEvent(const FHVGhostEventRow& Row, const FTransform* OverrideTransform)
{
	bool bTriggered = false;
	switch (Row.EventType)
	{
	case EHVGhostEventType::WallCrawl:      bTriggered = Trigger_WallCrawl(Row, OverrideTransform); break;
	case EHVGhostEventType::CeilingCrawl:   bTriggered = Trigger_CeilingCrawl(Row, OverrideTransform); break;
	case EHVGhostEventType::ContortWalk:    bTriggered = Trigger_ContortWalk(Row, OverrideTransform); break;
	case EHVGhostEventType::TVCrawlOut:     bTriggered = Trigger_TVCrawlOut(Row, OverrideTransform); break;
	case EHVGhostEventType::ElevatorHang:   bTriggered = Trigger_ElevatorHang(Row, OverrideTransform); break;
	case EHVGhostEventType::PassBehind:     bTriggered = Trigger_PassBehind(Row, OverrideTransform); break;
	case EHVGhostEventType::StandBehind:    bTriggered = Trigger_StandBehind(Row, OverrideTransform); break;
	case EHVGhostEventType::DistantStare:   bTriggered = Trigger_DistantStare(Row, OverrideTransform); break;
	case EHVGhostEventType::DoorPeek:       bTriggered = Trigger_DoorPeek(Row, OverrideTransform); break;
	case EHVGhostEventType::LightsOutFace:  bTriggered = Trigger_LightsOutFace(Row, OverrideTransform); break;
	case EHVGhostEventType::StairwellHang:  bTriggered = Trigger_StairwellHang(Row, OverrideTransform); break;
	case EHVGhostEventType::LobbyCrawl:     bTriggered = Trigger_LobbyCrawl(Row, OverrideTransform); break;
	default: break;
	}

	if (bTriggered)
	{
		FinishEventSetup(Row);
	}
	return bTriggered;
}

void UHVGhostDirector::FinishEventSetup(const FHVGhostEventRow& Row)
{
	ActiveEventType = Row.EventType;
	LastEventType = Row.EventType;
	bActiveDespawnWhenSeen = Row.bDespawnWhenSeen;
	ActiveEventElapsed = 0.f;
	bDespawnInProgress = false;

	// 強度越高冷卻越短。
	PendingCooldown = Row.CooldownAfter / GetIntensityScale();
	ScheduleDespawn(Row.Duration);

	OnGhostEventTriggered.Broadcast(Row.EventType);
	UE_LOG(LogHotelVictoria, Log, TEXT("女鬼事件觸發：%s（樓層 %d，持續 %.1fs，冷卻 %.1fs）"),
		*UEnum::GetValueAsString(Row.EventType), GetPlayerFloor(), Row.Duration, PendingCooldown);
}

// ---------------------------------------------------------------------------
// 生成 / 消散
// ---------------------------------------------------------------------------

AHVGhostCharacter* UHVGhostDirector::SpawnGhost(const FTransform& Transform, EHVGhostMode Mode, float FadeInTime)
{
	UWorld* World = GetWorld();
	if (!World || !GhostClass)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AHVGhostCharacter* Ghost = World->SpawnActor<AHVGhostCharacter>(GhostClass, Transform, Params);
	if (!Ghost)
	{
		return nullptr;
	}

	Ghost->SetGhostMode(Mode);
	Ghost->FadeIn(FadeInTime);
	Ghost->OnFadeOutFinished.AddUniqueDynamic(this, &UHVGhostDirector::HandleGhostFadedOut);

	ActiveGhost = Ghost;
	return Ghost;
}

void UHVGhostDirector::ScheduleDespawn(float Duration)
{
	GetWorld()->GetTimerManager().SetTimer(
		DespawnTimerHandle, this, &UHVGhostDirector::HandleDespawnTimer, FMath::Max(Duration, 0.5f), false);
}

void UHVGhostDirector::HandleDespawnTimer()
{
	if (ActiveGhost && !bDespawnInProgress)
	{
		bDespawnInProgress = true;
		ActiveGhost->FadeOut(0.8f);
	}
}

void UHVGhostDirector::HandleGhostFadedOut()
{
	// 熄燈貼臉結束後請關卡藍圖復燈。
	if (ActiveEventType == EHVGhostEventType::LightsOutFace)
	{
		OnLightsOutRequested.Broadcast(false);
	}

	if (ActiveGhost)
	{
		ActiveGhost->Destroy();
		ActiveGhost = nullptr;
	}

	ActiveEventType = EHVGhostEventType::None;
	bDespawnInProgress = false;
	CooldownRemaining = PendingCooldown;
}

void UHVGhostDirector::UpdateSeenCheck(float /*DeltaTime*/)
{
	if (!bActiveDespawnWhenSeen || !ActiveGhost || bDespawnInProgress || ActiveEventElapsed < 0.5f)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetPlayerView(ViewLocation, ViewRotation))
	{
		return;
	}

	const FVector ToGhost = ActiveGhost->GetActorLocation() - ViewLocation;
	const float Distance = ToGhost.Size();
	if (Distance > 1500.f)
	{
		return;
	}

	const float Dot = FVector::DotProduct(ViewRotation.Vector(), ToGhost.GetSafeNormal());
	if (Dot > 0.55f)
	{
		// 一回頭就快速消散——「妳確定剛剛看到了什麼嗎？」
		GetWorld()->GetTimerManager().ClearTimer(DespawnTimerHandle);
		bDespawnInProgress = true;
		ActiveGhost->FadeOut(0.45f);
	}
}

// ---------------------------------------------------------------------------
// 共用空間查詢
// ---------------------------------------------------------------------------

bool UHVGhostDirector::GetPlayerView(FVector& OutLocation, FRotator& OutRotation) const
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->GetPlayerViewPoint(OutLocation, OutRotation);
		return true;
	}
	return false;
}

bool UHVGhostDirector::FindTaggedSpawn(FName Tag, FTransform& OutTransform) const
{
	if (Tag.IsNone())
	{
		return false;
	}

	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Pawn)
	{
		return false;
	}
	const FVector PlayerLocation = Pawn->GetActorLocation();

	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ATargetPoint::StaticClass(), Tag, Tagged);

	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (AActor* Candidate : Tagged)
	{
		// 限定同樓層（高低差小於 0.6 個層高）。
		if (FMath::Abs(Candidate->GetActorLocation().Z - PlayerLocation.Z) > FloorHeight * 0.6f)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared2D(Candidate->GetActorLocation(), PlayerLocation);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	if (Best)
	{
		OutTransform = Best->GetActorTransform();
		return true;
	}
	return false;
}

bool UHVGhostDirector::ResolveSpawnTransform(const FHVGhostEventRow& Row, const FTransform* Override,
	const FVector& RelativeFallbackOffset, FTransform& OutTransform) const
{
	if (Override)
	{
		OutTransform = *Override;
		return true;
	}

	if (FindTaggedSpawn(Row.SpawnTag, OutTransform))
	{
		return true;
	}

	if (Row.bRequiresSpawnTag)
	{
		return false; // 必須對位的事件，缺定位點就放棄
	}

	// 保底：以玩家視角座標系擺放（X=前、Y=右、Z=上）。
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetPlayerView(ViewLocation, ViewRotation))
	{
		return false;
	}

	FRotator YawOnly(0.f, ViewRotation.Yaw, 0.f);
	const FVector WorldOffset = YawOnly.RotateVector(RelativeFallbackOffset);
	OutTransform = FTransform(YawOnly, ViewLocation + WorldOffset);
	return true;
}

// ---------------------------------------------------------------------------
// 12 種事件入口：Spawn 位置 + 姿態 + 參數化位移 + （定時）消散
// ---------------------------------------------------------------------------

bool UHVGhostDirector::Trigger_WallCrawl(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 走廊側牆面：自腰部高度沿牆向上爬，爬到近天花板處由計時器淡出。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector(380.f, 170.f, -60.f), Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::CrawlWall, 0.6f);
	if (!Ghost)
	{
		return false;
	}
	const FVector From = Spawn.GetLocation();
	Ghost->StartParametricMove(From, From + FVector(0.f, 0.f, 300.f), Row.Duration * 0.8f, MoveEaseCurve);
	return true;
}

bool UHVGhostDirector::Trigger_CeilingCrawl(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 天花板：自玩家前方 5m 上空朝玩家頭頂正上方爬近。
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetPlayerView(ViewLocation, ViewRotation))
	{
		return false;
	}
	const FVector Forward = FRotator(0.f, ViewRotation.Yaw, 0.f).Vector();
	const float CeilingZ = ViewLocation.Z + 170.f;

	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector(500.f, 0.f, 170.f), Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::CrawlCeiling, 0.7f);
	if (!Ghost)
	{
		return false;
	}
	const FVector To = FVector(ViewLocation.X, ViewLocation.Y, CeilingZ) + Forward * 120.f;
	Ghost->StartParametricMove(Spawn.GetLocation(), To, Row.Duration * 0.85f, MoveEaseCurve);
	return true;
}

bool UHVGhostDirector::Trigger_ContortWalk(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 反折行走：自走廊深處以不自然姿態緩慢逼近，停在 3m 外凝住。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector(750.f, 0.f, -150.f), Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::ContortWalk, 1.0f);
	if (!Ghost)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerView(ViewLocation, ViewRotation);
	const FVector From = Spawn.GetLocation();
	FVector To = ViewLocation + FRotator(0.f, ViewRotation.Yaw, 0.f).Vector() * 300.f;
	To.Z = From.Z;

	Ghost->FacePlayer();
	Ghost->StartParametricMove(From, To, Row.Duration * 0.9f, MoveEaseCurve);
	return true;
}

bool UHVGhostDirector::Trigger_TVCrawlOut(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 自老電視螢幕爬出（必須有 TVPoint 定位點：位於螢幕前緣、X 朝房間內）。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector::ZeroVector, Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::CrawlFloor, 0.5f);
	if (!Ghost)
	{
		return false;
	}
	const FVector From = Spawn.GetLocation();
	const FVector To = From + Spawn.GetRotation().GetForwardVector() * 180.f;
	Ghost->StartParametricMove(From, To, Row.Duration * 0.7f, MoveEaseCurve);
	return true;
}

bool UHVGhostDirector::Trigger_ElevatorHang(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 電梯倒掛：自門楣垂下凝視，小幅下沉增加「還在靠近」的壓迫感。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector(150.f, 0.f, 200.f), Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::HangInverted, 0.35f);
	if (!Ghost)
	{
		return false;
	}
	Ghost->FacePlayer();
	const FVector From = Spawn.GetLocation();
	Ghost->StartParametricMove(From, From - FVector(0.f, 0.f, 45.f), Row.Duration, MoveEaseCurve);
	return true;
}

bool UHVGhostDirector::Trigger_PassBehind(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 背後經過：自玩家背後左側橫越到右側；被直視即提前消散（資料表 bDespawnWhenSeen=true）。
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetPlayerView(ViewLocation, ViewRotation))
	{
		return false;
	}
	const FRotator YawOnly(0.f, ViewRotation.Yaw, 0.f);
	const FVector Back = -YawOnly.Vector();
	const FVector Right = YawOnly.RotateVector(FVector::RightVector);
	const FVector BaseLocation = ViewLocation + Back * 260.f - FVector(0.f, 0.f, 150.f);

	FTransform Spawn(YawOnly, BaseLocation - Right * 220.f);
	if (Override)
	{
		Spawn = *Override;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::Walk, 0.4f);
	if (!Ghost)
	{
		return false;
	}
	Ghost->StartParametricMove(Spawn.GetLocation(), BaseLocation + Right * 220.f, Row.Duration * 0.9f, MoveEaseCurve);
	return true;
}

bool UHVGhostDirector::Trigger_StandBehind(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 背後佇立：在玩家正後方 2m 靜立面向玩家；回頭即消散。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector(-200.f, 0.f, -150.f), Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::Stand, 0.8f);
	if (!Ghost)
	{
		return false;
	}
	Ghost->FacePlayer();
	return true;
}

bool UHVGhostDirector::Trigger_DistantStare(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 遠端凝視：走廊另一端 12m 處靜立直視，長時間不動，淡出時機由 Duration 決定。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector(1200.f, 0.f, -150.f), Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::Stand, 1.4f);
	if (!Ghost)
	{
		return false;
	}
	Ghost->FacePlayer();
	return true;
}

bool UHVGhostDirector::Trigger_DoorPeek(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 門縫探頭：需 DoorPeekPoint 定位點（門縫內側，X 朝走廊）；探出 25cm 後縮回由淡出處理。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector::ZeroVector, Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::DoorPeek, 0.6f);
	if (!Ghost)
	{
		return false;
	}
	const FVector From = Spawn.GetLocation();
	const FVector To = From + Spawn.GetRotation().GetForwardVector() * 25.f;
	Ghost->StartParametricMove(From, To, Row.Duration * 0.5f, MoveEaseCurve);
	return true;
}

bool UHVGhostDirector::Trigger_LightsOutFace(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 熄燈貼臉：先廣播熄燈（關卡藍圖關掉本層壁燈），女鬼直接出現在鼻尖前 80cm；
	// 事件結束（HandleGhostFadedOut）自動廣播復燈。
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetPlayerView(ViewLocation, ViewRotation))
	{
		return false;
	}

	OnLightsOutRequested.Broadcast(true);

	const FRotator YawOnly(0.f, ViewRotation.Yaw, 0.f);
	FVector SpawnLocation = ViewLocation + YawOnly.Vector() * 80.f - FVector(0.f, 0.f, 150.f);
	FRotator FaceBack = YawOnly;
	FaceBack.Yaw += 180.f; // 面向玩家

	FTransform Spawn(FaceBack, SpawnLocation);
	if (Override)
	{
		Spawn = *Override;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::FaceClose, 0.25f);
	if (!Ghost)
	{
		OnLightsOutRequested.Broadcast(false); // 生成失敗立即復燈，避免永夜
		return false;
	}
	return true;
}

bool UHVGhostDirector::Trigger_StairwellHang(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 樓梯間倒吊：需 StairHangPoint 定位點（梯井中央、天花板下緣），小幅擺盪下沉。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector::ZeroVector, Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::HangInverted, 0.5f);
	if (!Ghost)
	{
		return false;
	}
	Ghost->FacePlayer();
	const FVector From = Spawn.GetLocation();
	Ghost->StartParametricMove(From, From - FVector(0.f, 0.f, 40.f), Row.Duration, MoveEaseCurve);
	return true;
}

bool UHVGhostDirector::Trigger_LobbyCrawl(const FHVGhostEventRow& Row, const FTransform* Override)
{
	// 大廳爬行：需 LobbyPoint 定位點（大廳一側、X 朝對側），沿地面長距離爬過。
	FTransform Spawn;
	if (!ResolveSpawnTransform(Row, Override, FVector::ZeroVector, Spawn))
	{
		return false;
	}
	AHVGhostCharacter* Ghost = SpawnGhost(Spawn, EHVGhostMode::CrawlFloor, 0.8f);
	if (!Ghost)
	{
		return false;
	}
	const FVector From = Spawn.GetLocation();
	const FVector To = From + Spawn.GetRotation().GetForwardVector() * 600.f;
	Ghost->StartParametricMove(From, To, Row.Duration * 0.95f, MoveEaseCurve);
	return true;
}
