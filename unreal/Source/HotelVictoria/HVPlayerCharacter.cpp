// 第一人稱玩家角色實作：Enhanced Input 綁定、手電筒閃爍、互動射線與 UI 提示廣播。
#include "HVPlayerCharacter.h"

#include "HotelVictoria.h"
#include "HVGameMode.h"
#include "HVInteractable.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/HitResult.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

AHVPlayerCharacter::AHVPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	// 第一人稱：角色 Yaw 跟隨控制器，Pitch 交給相機。
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // 約 1.6m 視線高度
	Camera->bUsePawnControlRotation = true;

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(Camera);
	Flashlight->SetRelativeLocation(FVector(10.f, 8.f, -12.f)); // 略偏右下，模擬手持
	Flashlight->SetIntensity(FlashlightBaseIntensity);
	Flashlight->SetAttenuationRadius(2200.f);
	Flashlight->SetInnerConeAngle(16.f);
	Flashlight->SetOuterConeAngle(30.f);
	Flashlight->SetCastShadows(true);
	// 體積霧中拉出可見光錐（黑悟空式氛圍的關鍵之一）。
	Flashlight->SetVolumetricScatteringIntensity(2.0f);

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 1400.f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;

	FlickerSeed = FMath::FRandRange(0.f, 1000.f);
}

void AHVPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// 取得 Enhanced Input 的 Local Player Subsystem 並掛上預設 Mapping Context。
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, /*Priority=*/0);
			}
			else
			{
				UE_LOG(LogHotelVictoria, Warning,
					TEXT("DefaultMappingContext 未指定——請在 BP_HVPlayer 設定 IMC_Default。"));
			}
		}
	}
}

void AHVPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHVPlayerCharacter::HandleMove);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHVPlayerCharacter::HandleLook);
		}
		if (SprintAction)
		{
			EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AHVPlayerCharacter::HandleSprintStart);
			EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHVPlayerCharacter::HandleSprintStop);
		}
		if (FlashlightAction)
		{
			EIC->BindAction(FlashlightAction, ETriggerEvent::Started, this, &AHVPlayerCharacter::HandleToggleFlashlight);
		}
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AHVPlayerCharacter::HandleInteract);
		}
	}
	else
	{
		UE_LOG(LogHotelVictoria, Error,
			TEXT("PlayerInputComponent 不是 UEnhancedInputComponent——請確認 DefaultInput.ini 設定。"));
	}
}

void AHVPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFlashlightFlicker(DeltaSeconds);
	UpdateInteractionTrace();
}

void AHVPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveVector = Value.Get<FVector2D>();
	if (Controller)
	{
		AddMovementInput(GetActorForwardVector(), MoveVector.Y);
		AddMovementInput(GetActorRightVector(), MoveVector.X);
	}
}

void AHVPlayerCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(LookVector.X);
		AddControllerPitchInput(LookVector.Y);
	}
}

void AHVPlayerCharacter::HandleSprintStart(const FInputActionValue& /*Value*/)
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AHVPlayerCharacter::HandleSprintStop(const FInputActionValue& /*Value*/)
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AHVPlayerCharacter::HandleToggleFlashlight(const FInputActionValue& /*Value*/)
{
	SetFlashlightOn(!bFlashlightOn);
}

void AHVPlayerCharacter::SetFlashlightOn(bool bOn)
{
	bFlashlightOn = bOn;
	if (Flashlight)
	{
		Flashlight->SetVisibility(bOn);
	}
}

void AHVPlayerCharacter::HandleInteract(const FInputActionValue& /*Value*/)
{
	AActor* Target = FocusedActor.Get();
	if (Target && Target->Implements<UHVInteractable>() && IHVInteractable::Execute_CanInteract(Target))
	{
		IHVInteractable::Execute_Interact(Target, this);
	}
}

void AHVPlayerCharacter::UpdateFlashlightFlicker(float /*DeltaSeconds*/)
{
	if (!Flashlight || !bFlashlightOn)
	{
		return;
	}

	const float Time = GetWorld()->GetTimeSeconds();

	// 細微高頻抖動：老舊手電筒接觸不良的質感。
	const float Noise = FMath::PerlinNoise1D(Time * FlickerFrequency + FlickerSeed);

	// 偶發短暫壓暗（低頻噪聲越過門檻時亮度掉到 45%）。
	const float SlowNoise = FMath::PerlinNoise1D(Time * 0.7f + FlickerSeed * 2.f);
	const float DropScale = (SlowNoise > 0.62f) ? 0.45f : 1.f;

	Flashlight->SetIntensity(FlashlightBaseIntensity * DropScale * (1.f + Noise * FlickerAmount));
}

void AHVPlayerCharacter::UpdateInteractionTrace()
{
	AHVGameMode* GameMode = GetWorld()->GetAuthGameMode<AHVGameMode>();

	// 非探索狀態（讀檔、運鏡、終章）不做互動偵測。
	if (!GameMode || GameMode->GetGameState() != EHVGameState::Play || !Camera)
	{
		if (FocusedActor.IsValid())
		{
			FocusedActor = nullptr;
			if (GameMode)
			{
				GameMode->BroadcastInteractHint(false, FText::GetEmpty());
			}
		}
		return;
	}

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * InteractDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HVInteractTrace), /*bTraceComplex=*/false, this);
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	AActor* HitActor = Hit.GetActor();
	const bool bInteractable =
		HitActor &&
		HitActor->Implements<UHVInteractable>() &&
		IHVInteractable::Execute_CanInteract(HitActor);

	AActor* NewFocus = bInteractable ? HitActor : nullptr;
	if (NewFocus != FocusedActor.Get())
	{
		FocusedActor = NewFocus;
		if (NewFocus)
		{
			GameMode->BroadcastInteractHint(true, IHVInteractable::Execute_GetInteractHint(NewFocus));
		}
		else
		{
			GameMode->BroadcastInteractHint(false, FText::GetEmpty());
		}
	}
}
