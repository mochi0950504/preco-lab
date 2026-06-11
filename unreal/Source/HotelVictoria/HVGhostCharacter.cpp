// 女鬼角色實作：溶解淡入淡出、姿態切換、曲線參數化位移。
#include "HVGhostCharacter.h"

#include "HotelVictoria.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AHVGhostCharacter::AHVGhostCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 永不傷害也永不阻擋玩家：關閉膠囊碰撞，移動全靠導演的參數化位移。
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 飛行模式 + 零重力：天花板爬行、倒吊等姿態不會往下掉。
	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;

	// 不投射假影（鬼影由溶解材質與燈光營造），避免半透明階段陰影穿幫。
	GetMesh()->SetCastShadow(false);
}

void AHVGhostCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultMeshRelLoc = GetMesh()->GetRelativeLocation();
	DefaultMeshRelRot = GetMesh()->GetRelativeRotation();

	// 為每個材質槽建立 MID，之後統一寫入 Dissolve 純量參數。
	USkeletalMeshComponent* MeshComp = GetMesh();
	const int32 NumMaterials = MeshComp->GetNumMaterials();
	DissolveMIDs.Reserve(NumMaterials);
	for (int32 Index = 0; Index < NumMaterials; ++Index)
	{
		if (UMaterialInstanceDynamic* MID = MeshComp->CreateAndSetMaterialInstanceDynamic(Index))
		{
			DissolveMIDs.Add(MID);
		}
	}

	// 初始全消散（不可見），等導演 FadeIn。
	DissolveValue = 1.f;
	DissolveTarget = 1.f;
	ApplyDissolveValue();
}

void AHVGhostCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateDissolve(DeltaSeconds);
	UpdateParametricMove(DeltaSeconds);
}

void AHVGhostCharacter::SetGhostMode(EHVGhostMode NewMode)
{
	if (GhostMode == NewMode)
	{
		return;
	}
	GhostMode = NewMode;

	// 倒吊 / 天花板爬行：以 Roll 180 翻轉 Mesh 作為佔位姿態，正式版交給 AnimBP / Control Rig。
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (NewMode == EHVGhostMode::HangInverted || NewMode == EHVGhostMode::CrawlCeiling)
	{
		FRotator Flipped = DefaultMeshRelRot;
		Flipped.Roll += 180.f;
		MeshComp->SetRelativeRotation(Flipped);
		MeshComp->SetRelativeLocation(DefaultMeshRelLoc + FVector(0.f, 0.f, 176.f));
	}
	else
	{
		MeshComp->SetRelativeRotation(DefaultMeshRelRot);
		MeshComp->SetRelativeLocation(DefaultMeshRelLoc);
	}

	OnGhostModeChanged(NewMode);
}

void AHVGhostCharacter::FadeIn(float Duration)
{
	bFadingOut = false;
	DissolveTarget = 0.f;
	DissolveSpeed = 1.f / FMath::Max(Duration, 0.05f);
}

void AHVGhostCharacter::FadeOut(float Duration)
{
	bFadingOut = true;
	DissolveTarget = 1.f;
	DissolveSpeed = 1.f / FMath::Max(Duration, 0.05f);
}

void AHVGhostCharacter::UpdateDissolve(float DeltaSeconds)
{
	if (FMath::IsNearlyEqual(DissolveValue, DissolveTarget, 0.001f))
	{
		return;
	}

	DissolveValue = FMath::FInterpConstantTo(DissolveValue, DissolveTarget, DeltaSeconds, DissolveSpeed);
	ApplyDissolveValue();

	if (bFadingOut && FMath::IsNearlyEqual(DissolveValue, 1.f, 0.001f))
	{
		bFadingOut = false;
		SetGhostMode(EHVGhostMode::Hidden);
		OnFadeOutFinished.Broadcast();
	}
}

void AHVGhostCharacter::ApplyDissolveValue()
{
	for (UMaterialInstanceDynamic* MID : DissolveMIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(DissolveParamName, DissolveValue);
		}
	}
	// 材質尚未接溶解節點時的保底：全消散直接隱形。
	SetActorHiddenInGame(DissolveValue >= 0.999f);
}

void AHVGhostCharacter::StartParametricMove(const FVector& From, const FVector& To, float Duration, UCurveFloat* EaseCurve)
{
	MoveFrom = From;
	MoveTo = To;
	MoveDuration = FMath::Max(Duration, 0.05f);
	MoveElapsed = 0.f;
	MoveEaseCurve = EaseCurve;
	bMoving = true;

	SetActorLocation(From);
}

void AHVGhostCharacter::UpdateParametricMove(float DeltaSeconds)
{
	if (!bMoving)
	{
		CurrentMoveSpeed = 0.f;
		return;
	}

	MoveElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(MoveElapsed / MoveDuration, 0.f, 1.f);
	const float Eased = MoveEaseCurve ? MoveEaseCurve->GetFloatValue(Alpha) : Alpha;

	const FVector PrevLocation = GetActorLocation();
	const FVector NewLocation = FMath::Lerp(MoveFrom, MoveTo, Eased);
	SetActorLocation(NewLocation);
	CurrentMoveSpeed = (DeltaSeconds > KINDA_SMALL_NUMBER)
		? (NewLocation - PrevLocation).Size() / DeltaSeconds
		: 0.f;

	if (Alpha >= 1.f)
	{
		bMoving = false;
		OnMoveFinished.Broadcast();
	}
}

void AHVGhostCharacter::FacePlayer()
{
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToPlayer = CameraManager->GetCameraLocation() - GetActorLocation();
		FRotator LookRot = ToPlayer.Rotation();
		LookRot.Pitch = 0.f;
		LookRot.Roll = 0.f;
		SetActorRotation(LookRot);
	}
}
