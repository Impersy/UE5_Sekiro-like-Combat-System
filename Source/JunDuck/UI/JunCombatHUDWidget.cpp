#include "UI/JunCombatHUDWidget.h"

#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Character/Monster/TutorialNPC/JunTutorialNPC.h"
#include "PlayerController/JunPlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "MediaPlayer.h"
#include "MediaSource.h"

void UJunCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerDelayedHealthPercent = PlayerHealthPercent;
	BossTargetHealthPercent = BossHealthPercent;
	BossDelayedHealthPercent = BossHealthPercent;
	PlayerDelayedHealthDelayRemainTime = 0.f;
	BossDelayedHealthDelayRemainTime = 0.f;

	ApplyPlayerHealthBars();
	ApplyBossHealthBars();
	ApplyPostureBars();
	ApplyPotionWidgets();
	ApplyControlsVisibility();
	ApplyBossHealthVisibility();
	ApplyBossLifeWidgets();
	HideDialogue();
	ApplyWidgetOpacity(Tutorial_DimBlack, 0.f);
	HidePauseMenu();
	BeginRestartFadeFromBlack();
	ApplyMapNoticeVisibility();
	HideTutorialGuides();
	HideTutorialTask();
	if (PlayerPostureGuideMediaPlayer)
	{
		PlayerPostureGuideMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UJunCombatHUDWidget::HandleTutorialGuideMediaOpened);
		PlayerPostureGuideMediaPlayer->OnMediaOpened.AddDynamic(this, &UJunCombatHUDWidget::HandleTutorialGuideMediaOpened);
	}
	if (DangerAttackGuideSubMediaPlayer)
	{
		DangerAttackGuideSubMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UJunCombatHUDWidget::HandleTutorialGuideSubMediaOpened);
		DangerAttackGuideSubMediaPlayer->OnMediaOpened.AddDynamic(this, &UJunCombatHUDWidget::HandleTutorialGuideSubMediaOpened);
	}
	if (Player_RedGlow_Root)
	{
		Player_RedGlow_Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Player_RedGlow_Root->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
		Player_RedGlow_Root->SetRenderOpacity(0.f);
		Player_RedGlow_Root->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Boss_RedGlow_Root)
	{
		Boss_RedGlow_Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Boss_RedGlow_Root->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
		Boss_RedGlow_Root->SetRenderOpacity(0.f);
		Boss_RedGlow_Root->SetVisibility(ESlateVisibility::Hidden);
	}
	ApplyWidgetOpacity(Boss_Clear, 0.f);
	HideDeathUI();
	OnBossHealthVisibilityChanged(bBossHealthVisible);
	OnPlayerDelayedHealthChanged(PlayerDelayedHealthPercent);
	OnBossDelayedHealthChanged(BossDelayedHealthPercent);
}

void UJunCombatHUDWidget::NativeDestruct()
{
	if (PlayerPostureGuideMediaPlayer)
	{
		PlayerPostureGuideMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UJunCombatHUDWidget::HandleTutorialGuideMediaOpened);
		PlayerPostureGuideMediaPlayer->Close();
	}
	if (DangerAttackGuideSubMediaPlayer)
	{
		DangerAttackGuideSubMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UJunCombatHUDWidget::HandleTutorialGuideSubMediaOpened);
		DangerAttackGuideSubMediaPlayer->Close();
	}

	Super::NativeDestruct();
}

void UJunCombatHUDWidget::ShowPlayerPostureGuide()
{
	HideTutorialGuides();
	if (Tutorial_PlayerPosture_Guide)
	{
		Tutorial_PlayerPosture_Guide->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Video_1)
	{
		Video_1->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (PlayerPostureGuideMediaPlayer && PlayerPostureGuideMediaSource)
	{
		PlayerPostureGuideMediaPlayer->Close();
		PlayerPostureGuideMediaPlayer->SetLooping(true);
		PlayerPostureGuideMediaPlayer->OpenSource(PlayerPostureGuideMediaSource);
	}
}

FReply UJunCombatHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Pause_Root && Pause_Root->GetVisibility() == ESlateVisibility::Visible &&
		!bRestartFadeToBlackActive && !bRestartFadeFromBlackActive &&
		InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (Restart_Game && Restart_Game->IsHovered())
		{
			HandleRestartClicked();
			return FReply::Handled();
		}

		if (Quit_Game && Quit_Game->IsHovered())
		{
			HandleQuitClicked();
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UJunCombatHUDWidget::HidePlayerPostureGuide()
{
	HideTutorialGuides();
}

void UJunCombatHUDWidget::ShowMonsterPostureGuide()
{
	HideTutorialGuides();
	if (Tutorial_MonsterPosture_Guide)
	{
		Tutorial_MonsterPosture_Guide->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Video_2)
	{
		Video_2->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (PlayerPostureGuideMediaPlayer && MonsterPostureGuideMediaSource)
	{
		PlayerPostureGuideMediaPlayer->SetLooping(true);
		PlayerPostureGuideMediaPlayer->OpenSource(MonsterPostureGuideMediaSource);
	}
}

void UJunCombatHUDWidget::ShowDangerAttackGuide()
{
	HideTutorialGuides();
	if (Tutorial_DangerAttack_Guide)
	{
		Tutorial_DangerAttack_Guide->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Video_3)
	{
		Video_3->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Video_3_Sub)
	{
		Video_3_Sub->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (PlayerPostureGuideMediaPlayer && DangerAttackGuideMediaSource)
	{
		PlayerPostureGuideMediaPlayer->SetLooping(true);
		PlayerPostureGuideMediaPlayer->OpenSource(DangerAttackGuideMediaSource);
	}
	if (DangerAttackGuideSubMediaPlayer && DangerAttackGuideSubMediaSource)
	{
		DangerAttackGuideSubMediaPlayer->SetLooping(true);
		DangerAttackGuideSubMediaPlayer->OpenSource(DangerAttackGuideSubMediaSource);
	}
}

void UJunCombatHUDWidget::ShowAirParryGuide()
{
	HideTutorialGuides();
	if (Tutorial_AirParry_Guide)
	{
		Tutorial_AirParry_Guide->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Video_4)
	{
		Video_4->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (PlayerPostureGuideMediaPlayer && AirParryGuideMediaSource)
	{
		PlayerPostureGuideMediaPlayer->SetLooping(true);
		PlayerPostureGuideMediaPlayer->OpenSource(AirParryGuideMediaSource);
	}
}

void UJunCombatHUDWidget::HideTutorialGuides()
{
	if (PlayerPostureGuideMediaPlayer)
	{
		PlayerPostureGuideMediaPlayer->Close();
	}
	if (DangerAttackGuideSubMediaPlayer)
	{
		DangerAttackGuideSubMediaPlayer->Close();
	}
	if (Tutorial_PlayerPosture_Guide)
	{
		Tutorial_PlayerPosture_Guide->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Video_1)
	{
		Video_1->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Tutorial_MonsterPosture_Guide)
	{
		Tutorial_MonsterPosture_Guide->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Video_2)
	{
		Video_2->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Tutorial_DangerAttack_Guide)
	{
		Tutorial_DangerAttack_Guide->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Video_3)
	{
		Video_3->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Video_3_Sub)
	{
		Video_3_Sub->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Tutorial_AirParry_Guide)
	{
		Tutorial_AirParry_Guide->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Video_4)
	{
		Video_4->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::HandleTutorialGuideMediaOpened(FString OpenedUrl)
{
	if (PlayerPostureGuideMediaPlayer)
	{
		PlayerPostureGuideMediaPlayer->Play();
	}
}

void UJunCombatHUDWidget::HandleTutorialGuideSubMediaOpened(FString OpenedUrl)
{
	if (DangerAttackGuideSubMediaPlayer)
	{
		DangerAttackGuideSubMediaPlayer->Play();
	}
}

void UJunCombatHUDWidget::ShowTutorialTask(int32 TutorialTaskType)
{
	HideTutorialTask();

	UWidget* TaskWidget = FindTutorialTaskWidget(TutorialTaskType);
	if (!TaskWidget)
	{
		return;
	}

	if (Tutorial_Task)
	{
		Tutorial_Task->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Task_BackGround)
	{
		Task_BackGround->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	SetTutorialTaskWidgetVisibility(TaskWidget, true);
}

void UJunCombatHUDWidget::HideTutorialTask()
{
	if (Tutorial_Task)
	{
		const int32 ChildCount = Tutorial_Task->GetChildrenCount();
		for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
		{
			if (UWidget* ChildWidget = Tutorial_Task->GetChildAt(ChildIndex))
			{
				ChildWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}

	// Keep the name-based fallback for task containers nested below another panel.
	for (int32 TaskType = static_cast<int32>(EJunTutorialTaskType::LockOn);
		 TaskType <= static_cast<int32>(EJunTutorialTaskType::Execution);
		 ++TaskType)
	{
		SetTutorialTaskWidgetVisibility(FindTutorialTaskWidget(TaskType), false);
	}

	if (Task_BackGround)
	{
		Task_BackGround->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Tutorial_Task)
	{
		Tutorial_Task->SetVisibility(ESlateVisibility::Hidden);
	}
}

UWidget* UJunCombatHUDWidget::FindTutorialTaskWidget(int32 TutorialTaskType) const
{
	const EJunTutorialTaskType TaskType = static_cast<EJunTutorialTaskType>(TutorialTaskType);
	FName WidgetName = NAME_None;

	switch (TaskType)
	{
	case EJunTutorialTaskType::LockOn:
		WidgetName = TEXT("Task_Lock On");
		break;
	case EJunTutorialTaskType::BasicAttackComboFinalHit:
		WidgetName = TEXT("Task_BasicAttack");
		break;
	case EJunTutorialTaskType::JumpAttackHit:
		WidgetName = TEXT("Task_JumpAttack");
		break;
	case EJunTutorialTaskType::DashAttackHit:
		WidgetName = TEXT("Task_DashAttack");
		break;
	case EJunTutorialTaskType::HeavyAttackComboHit:
		WidgetName = TEXT("Task_HeavyAttack_Combo");
		break;
	case EJunTutorialTaskType::HeavyChargeHit:
		WidgetName = TEXT("Task_HeavyAttack_Charge");
		break;
	case EJunTutorialTaskType::JigenSecondHit:
		WidgetName = TEXT("Task_Jigen");
		break;
	case EJunTutorialTaskType::DrinkPotion:
		WidgetName = TEXT("Task_Heal");
		break;
	case EJunTutorialTaskType::GuardPostureRecovery:
		WidgetName = TEXT("Task_Guarding");
		break;
	case EJunTutorialTaskType::ParryThreeTimes:
		WidgetName = TEXT("Task_Parry");
		break;
	case EJunTutorialTaskType::AirParryOnce:
		WidgetName = TEXT("Task_JumpParry");
		break;
	case EJunTutorialTaskType::MikiriCounter:
		WidgetName = TEXT("Task_Mikiri_Counter");
		break;
	case EJunTutorialTaskType::JumpCounterStomp:
		WidgetName = TEXT("Task_JumpCounterStomp");
		break;
	case EJunTutorialTaskType::Execution:
		WidgetName = TEXT("Task_Execution");
		break;
	default:
		return nullptr;
	}

	UWidget* Result = GetWidgetFromName(WidgetName);
	if (!Result && TaskType == EJunTutorialTaskType::LockOn)
	{
		Result = GetWidgetFromName(TEXT("Task_LockOn"));
	}
	return Result;
}

void UJunCombatHUDWidget::SetTutorialTaskWidgetVisibility(UWidget* TaskWidget, bool bVisible) const
{
	if (TaskWidget)
	{
		TaskWidget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateBossHealthFill(InDeltaTime);
	UpdateDelayedHealthBars(InDeltaTime);
	UpdateRedGlowEffects(InDeltaTime);
	UpdatePostureVisibility(InDeltaTime);
	UpdateBossClearUI(InDeltaTime);
	UpdateDeathUI(InDeltaTime);
	UpdateDialogueUI(InDeltaTime);
	UpdateTutorialDimBlackUI(InDeltaTime);
	UpdatePauseUI(InDeltaTime);
	UpdateMapNoticeUI(InDeltaTime);

	if (bDialogueCombatUIHidden)
	{
		for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Pair : DialogueCombatWidgetVisibilities)
		{
			if (UWidget* Widget = Pair.Key.Get())
			{
				Widget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UJunCombatHUDWidget::ShowPauseMenu()
{
	bRestartFadeToBlackActive = false;
	bRestartFadeFromBlackActive = false;
	bRestartLevelRequested = false;
	PauseDimBlackOpacity = 0.f;
	TargetPauseDimBlackOpacity = PauseDimBlackTargetOpacity;
	PauseOptionsOpacity = 1.f;
	TargetPauseOptionsOpacity = 1.f;
	RestartFullBlackHoldRemainTime = 0.f;
	if (Pause_Root)
	{
		Pause_Root->SetVisibility(ESlateVisibility::Visible);
	}
	SetPauseOptionsVisible(true);
	ApplyPauseOptionsOpacity();
	ApplyWidgetOpacity(Pause_DimBlack, PauseDimBlackOpacity, false);
	HandleRestartUnhovered();
	HandleQuitUnhovered();
}

void UJunCombatHUDWidget::HidePauseMenu()
{
	bRestartFadeToBlackActive = false;
	bRestartFadeFromBlackActive = false;
	bRestartLevelRequested = false;
	PauseDimBlackOpacity = 0.f;
	TargetPauseDimBlackOpacity = 0.f;
	PauseOptionsOpacity = 1.f;
	TargetPauseOptionsOpacity = 1.f;
	RestartFullBlackHoldRemainTime = 0.f;
	ApplyPauseOptionsOpacity();
	ApplyWidgetOpacity(Pause_DimBlack, 0.f, false);
	if (Pause_Root)
	{
		Pause_Root->SetVisibility(ESlateVisibility::Hidden);
	}
	HandleRestartUnhovered();
	HandleQuitUnhovered();
}

void UJunCombatHUDWidget::ShowMapNotice(int32 NoticeIndex)
{
	if (NoticeIndex != 1 && NoticeIndex != 2)
	{
		return;
	}

	ActiveMapNoticeIndex = NoticeIndex;
	MapNoticeOpacity = 0.f;
	TargetMapNoticeOpacity = 1.f;
	MapNoticeHoldRemainTime = MapNoticeHoldDuration;
	bMapNoticeHolding = false;
	ApplyMapNoticeVisibility();
}

void UJunCombatHUDWidget::UpdateMapNoticeUI(float DeltaTime)
{
	if (bInitialMapNoticePending)
	{
		InitialMapNoticeDelayRemainTime = FMath::Max(0.f, InitialMapNoticeDelayRemainTime - DeltaTime);
		if (InitialMapNoticeDelayRemainTime <= 0.f)
		{
			bInitialMapNoticePending = false;
			ShowMapNotice(1);
		}
	}

	if (ActiveMapNoticeIndex == 0)
	{
		return;
	}

	MapNoticeOpacity = MapNoticeFadeSpeed > 0.f
		? FMath::FInterpConstantTo(MapNoticeOpacity, TargetMapNoticeOpacity, DeltaTime, MapNoticeFadeSpeed)
		: TargetMapNoticeOpacity;

	if (TargetMapNoticeOpacity >= 1.f && FMath::IsNearlyEqual(MapNoticeOpacity, 1.f, 0.001f))
	{
		MapNoticeOpacity = 1.f;
		bMapNoticeHolding = true;
	}

	if (bMapNoticeHolding)
	{
		MapNoticeHoldRemainTime = FMath::Max(0.f, MapNoticeHoldRemainTime - DeltaTime);
		if (MapNoticeHoldRemainTime <= 0.f)
		{
			bMapNoticeHolding = false;
			TargetMapNoticeOpacity = 0.f;
		}
	}

	if (TargetMapNoticeOpacity <= 0.f && FMath::IsNearlyZero(MapNoticeOpacity, 0.001f))
	{
		MapNoticeOpacity = 0.f;
		ActiveMapNoticeIndex = 0;
	}

	ApplyMapNoticeVisibility();
}

void UJunCombatHUDWidget::ApplyMapNoticeVisibility()
{
	const bool bVisible = ActiveMapNoticeIndex != 0 && MapNoticeOpacity > 0.f;
	if (Map_Notice_Root)
	{
		Map_Notice_Root->SetRenderOpacity(MapNoticeOpacity);
		Map_Notice_Root->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (Map_Notice_UnderLine)
	{
		Map_Notice_UnderLine->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (Notice_1)
	{
		Notice_1->SetVisibility(bVisible && ActiveMapNoticeIndex == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (Notice_2)
	{
		Notice_2->SetVisibility(bVisible && ActiveMapNoticeIndex == 2 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::HandleRestartHovered()
{
	if (UnderLine_1)
	{
		UnderLine_1->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunCombatHUDWidget::HandleRestartUnhovered()
{
	if (UnderLine_1)
	{
		UnderLine_1->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::HandleRestartClicked()
{
	BeginRestartFadeToBlack();
}

void UJunCombatHUDWidget::HandleQuitHovered()
{
	if (UnderLine_2)
	{
		UnderLine_2->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunCombatHUDWidget::HandleQuitUnhovered()
{
	if (UnderLine_2)
	{
		UnderLine_2->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::HandleQuitClicked()
{
	if (AJunPlayerController* PlayerController = GetOwningPlayer<AJunPlayerController>())
	{
		PlayerController->QuitPausedGame();
	}
}

void UJunCombatHUDWidget::UpdatePauseUI(float DeltaTime)
{
	if (!Pause_Root || Pause_Root->GetVisibility() == ESlateVisibility::Hidden)
	{
		return;
	}

	if (bRestartFadeFromBlackActive && LevelIntroBlackHoldRemainTime > 0.f)
	{
		const float PreviousHoldRemainTime = LevelIntroBlackHoldRemainTime;
		LevelIntroBlackHoldRemainTime = FMath::Max(0.f, LevelIntroBlackHoldRemainTime - DeltaTime);
		PauseDimBlackOpacity = 1.f;
		ApplyWidgetOpacity(Pause_DimBlack, 1.f, false);
		if (PreviousHoldRemainTime > 0.f && LevelIntroBlackHoldRemainTime <= 0.f)
		{
			bInitialMapNoticePending = true;
			InitialMapNoticeDelayRemainTime = InitialMapNoticeDelay;
		}
		return;
	}

	const float ActiveFadeSpeed = bRestartFadeToBlackActive
		? RestartFadeToBlackSpeed
		: (bRestartFadeFromBlackActive ? RestartFadeFromBlackSpeed : PauseDimBlackFadeSpeed);
	PauseDimBlackOpacity = ActiveFadeSpeed > 0.f
		? FMath::FInterpTo(PauseDimBlackOpacity, TargetPauseDimBlackOpacity, DeltaTime, ActiveFadeSpeed)
		: TargetPauseDimBlackOpacity;
	if (FMath::IsNearlyEqual(PauseDimBlackOpacity, TargetPauseDimBlackOpacity, 0.001f))
	{
		PauseDimBlackOpacity = TargetPauseDimBlackOpacity;
	}
	ApplyWidgetOpacity(Pause_DimBlack, PauseDimBlackOpacity, false);

	PauseOptionsOpacity = ActiveFadeSpeed > 0.f
		? FMath::FInterpTo(PauseOptionsOpacity, TargetPauseOptionsOpacity, DeltaTime, ActiveFadeSpeed)
		: TargetPauseOptionsOpacity;
	if (FMath::IsNearlyEqual(PauseOptionsOpacity, TargetPauseOptionsOpacity, 0.001f))
	{
		PauseOptionsOpacity = TargetPauseOptionsOpacity;
	}
	ApplyPauseOptionsOpacity();

	if (bRestartFadeToBlackActive && PauseDimBlackOpacity >= 0.999f && !bRestartLevelRequested)
	{
		RestartFullBlackHoldRemainTime = FMath::Max(0.f, RestartFullBlackHoldRemainTime - DeltaTime);
		if (RestartFullBlackHoldRemainTime > 0.f)
		{
			return;
		}

		bRestartLevelRequested = true;
		if (AJunPlayerController* PlayerController = GetOwningPlayer<AJunPlayerController>())
		{
			PlayerController->RestartPausedGame();
		}
		return;
	}

	if (bRestartFadeFromBlackActive && PauseDimBlackOpacity <= 0.001f)
	{
		bRestartFadeFromBlackActive = false;
		if (Pause_Root)
		{
			Pause_Root->SetVisibility(ESlateVisibility::Hidden);
		}
		PauseOptionsOpacity = 1.f;
		TargetPauseOptionsOpacity = 1.f;
		SetPauseOptionsVisible(true);
		ApplyPauseOptionsOpacity();
		if (AJunPlayerController* PlayerController = GetOwningPlayer<AJunPlayerController>())
		{
			PlayerController->CompleteLevelIntroTransition();
		}
		return;
	}

	if (bRestartFadeToBlackActive || bRestartFadeFromBlackActive)
	{
		return;
	}

	if (Restart_Game && Restart_Game->IsHovered())
	{
		HandleRestartHovered();
	}
	else
	{
		HandleRestartUnhovered();
	}

	if (Quit_Game && Quit_Game->IsHovered())
	{
		HandleQuitHovered();
	}
	else
	{
		HandleQuitUnhovered();
	}
}

void UJunCombatHUDWidget::BeginRestartFadeToBlack()
{
	if (bRestartFadeToBlackActive || bRestartFadeFromBlackActive)
	{
		return;
	}

	bRestartFadeToBlackActive = true;
	bRestartLevelRequested = false;
	TargetPauseDimBlackOpacity = 1.f;
	TargetPauseOptionsOpacity = 0.f;
	RestartFullBlackHoldRemainTime = RestartFullBlackHoldDuration;
	if (Restart_Game)
	{
		Restart_Game->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Quit_Game)
	{
		Quit_Game->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	HandleRestartUnhovered();
	HandleQuitUnhovered();
}

void UJunCombatHUDWidget::BeginRestartFadeFromBlack()
{
	bRestartFadeToBlackActive = false;
	bRestartFadeFromBlackActive = true;
	bRestartLevelRequested = false;
	PauseDimBlackOpacity = 1.f;
	TargetPauseDimBlackOpacity = 0.f;
	PauseOptionsOpacity = 0.f;
	TargetPauseOptionsOpacity = 0.f;
	RestartFullBlackHoldRemainTime = 0.f;
	LevelIntroBlackHoldRemainTime = LevelIntroBlackHoldDuration;
	if (Pause_Root)
	{
		Pause_Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	SetPauseOptionsVisible(false);
	ApplyPauseOptionsOpacity();
	ApplyWidgetOpacity(Pause_DimBlack, 1.f, false);
}

void UJunCombatHUDWidget::SetPauseOptionsVisible(bool bVisible)
{
	const ESlateVisibility OptionVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
	if (Restart_Game)
	{
		Restart_Game->SetVisibility(OptionVisibility);
	}
	if (Quit_Game)
	{
		Quit_Game->SetVisibility(OptionVisibility);
	}
	if (!bVisible)
	{
		HandleRestartUnhovered();
		HandleQuitUnhovered();
	}
}

void UJunCombatHUDWidget::ApplyPauseOptionsOpacity() const
{
	if (Restart_Game)
	{
		Restart_Game->SetRenderOpacity(PauseOptionsOpacity);
	}
	if (Quit_Game)
	{
		Quit_Game->SetRenderOpacity(PauseOptionsOpacity);
	}
}

void UJunCombatHUDWidget::SetPotionCount(int32 CurrentPotionCount)
{
	PotionCount = FMath::Max(0, CurrentPotionCount);
	ApplyPotionWidgets();
}

void UJunCombatHUDWidget::ToggleControlsVisibility()
{
	bControlsVisible = !bControlsVisible;
	ApplyControlsVisibility();
}

void UJunCombatHUDWidget::ShowDialogue(const FText& DialogueText, const FText& SpeakerName)
{
	SetDialogueCombatUIHidden(true);
	CurrentDialogueText = DialogueText;
	CurrentDialogueSpeakerName = SpeakerName;
	TargetDialogueBackgroundOpacity = DialogueBackgroundTargetOpacity;
	ApplyDialogueText();
	ApplyDialogueVisibility();
}

void UJunCombatHUDWidget::HideDialogue()
{
	CurrentDialogueText = FText::GetEmpty();
	CurrentDialogueSpeakerName = FText::GetEmpty();
	TargetDialogueBackgroundOpacity = 0.f;
	ApplyDialogueText();
	ApplyDialogueVisibility();
	SetDialogueCombatUIHidden(false);
}

void UJunCombatHUDWidget::SetDialogueCombatUIHidden(bool bHidden)
{
	if (bDialogueCombatUIHidden == bHidden)
	{
		return;
	}

	bDialogueCombatUIHidden = bHidden;
	if (bHidden)
	{
		DialogueCombatWidgetVisibilities.Reset();
		CacheAndHideDialogueCombatWidget(BossHealthRoot);
		CacheAndHideDialogueCombatWidget(PlayerHealthRoot);
		CacheAndHideDialogueCombatWidget(BossPostureRoot);
		CacheAndHideDialogueCombatWidget(PlayerPostureRoot);
		CacheAndHideDialogueCombatWidget(Life_1_Root);
		CacheAndHideDialogueCombatWidget(Life_2_Root);
		CacheAndHideDialogueCombatWidget(Life_3_Root);
		CacheAndHideDialogueCombatWidget(Potion_Root);
		CacheAndHideDialogueCombatWidget(Weapon_Root);
		CacheAndHideDialogueCombatWidget(Controls_Root);
		CacheAndHideDialogueCombatWidget(Controls_Toggle);
		CacheAndHideDialogueCombatWidget(Clear_Root);
		CacheAndHideDialogueCombatWidget(Tutorial_Task);
		return;
	}

	for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Pair : DialogueCombatWidgetVisibilities)
	{
		if (UWidget* Widget = Pair.Key.Get())
		{
			Widget->SetVisibility(Pair.Value);
		}
	}
	DialogueCombatWidgetVisibilities.Reset();
}

void UJunCombatHUDWidget::CacheAndHideDialogueCombatWidget(UWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	DialogueCombatWidgetVisibilities.Add(Widget, Widget->GetVisibility());
	Widget->SetVisibility(ESlateVisibility::Hidden);
}

void UJunCombatHUDWidget::SetDialogueLine(const FText& DialogueText, const FText& SpeakerName)
{
	CurrentDialogueText = DialogueText;
	CurrentDialogueSpeakerName = SpeakerName;
	ApplyDialogueText();
	ApplyDialogueVisibility();
}

void UJunCombatHUDWidget::StartTutorialDimBlackFadeIn()
{
	TargetTutorialDimBlackOpacity = 1.f;
	if (Tutorial_DimBlack)
	{
		Tutorial_DimBlack->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunCombatHUDWidget::StartTutorialDimBlackFadeOut()
{
	TargetTutorialDimBlackOpacity = 0.f;
}

bool UJunCombatHUDWidget::IsTutorialDimBlackOpaque() const
{
	return TutorialDimBlackOpacity >= 0.99f;
}

bool UJunCombatHUDWidget::IsTutorialDimBlackHidden() const
{
	return TutorialDimBlackOpacity <= 0.01f && TargetTutorialDimBlackOpacity <= 0.f;
}

void UJunCombatHUDWidget::SetPlayerHealth(int32 CurrentHealth, int32 MaxHealth)
{
	const float PreviousHealthPercent = PlayerHealthPercent;
	PlayerHealthPercent = MaxHealth > 0
		? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.f, 1.f)
		: 0.f;

	if (PlayerHealthPercent < PreviousHealthPercent)
	{
		PlayerDelayedHealthDelayRemainTime = DelayedHealthStartDelay;
		PlayerDelayedHealthPercent = FMath::Max(PlayerDelayedHealthPercent, PreviousHealthPercent);
	}
	else if (PlayerHealthPercent > PlayerDelayedHealthPercent)
	{
		PlayerDelayedHealthPercent = PlayerHealthPercent;
		PlayerDelayedHealthDelayRemainTime = 0.f;
		OnPlayerDelayedHealthChanged(PlayerDelayedHealthPercent);
	}

	OnPlayerHealthChanged(CurrentHealth, MaxHealth, PlayerHealthPercent);
	ApplyPlayerHealthBars();
}

void UJunCombatHUDWidget::SetBossHealth(int32 CurrentHealth, int32 MaxHealth)
{
	const float PreviousHealthPercent = BossHealthPercent;
	BossTargetHealthPercent = MaxHealth > 0
		? FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth), 0.f, 1.f)
		: 0.f;

	if (BossTargetHealthPercent < BossHealthPercent || FMath::IsNearlyEqual(BossTargetHealthPercent, BossHealthPercent, 0.001f))
	{
		BossHealthPercent = BossTargetHealthPercent;
	}

	if (BossHealthPercent < PreviousHealthPercent)
	{
		BossDelayedHealthDelayRemainTime = DelayedHealthStartDelay;
		BossDelayedHealthPercent = FMath::Max(BossDelayedHealthPercent, PreviousHealthPercent);
	}
	else if (BossTargetHealthPercent > BossHealthPercent)
	{
		BossDelayedHealthDelayRemainTime = 0.f;
		BossDelayedHealthPercent = BossHealthPercent;
		OnBossDelayedHealthChanged(BossDelayedHealthPercent);
	}
	else if (BossTargetHealthPercent > BossDelayedHealthPercent && BossHealthPercent >= BossTargetHealthPercent)
	{
		BossDelayedHealthPercent = BossHealthPercent;
		BossDelayedHealthDelayRemainTime = 0.f;
		OnBossDelayedHealthChanged(BossDelayedHealthPercent);
	}

	OnBossHealthChanged(CurrentHealth, MaxHealth, BossHealthPercent);
	ApplyBossHealthBars();
}

void UJunCombatHUDWidget::SetBossLifeState(int32 CurrentLifeCount, int32 MaxLifeCount)
{
	BossMaxLifeCount = FMath::Clamp(MaxLifeCount, 0, 3);
	BossCurrentLifeCount = FMath::Clamp(CurrentLifeCount, 0, BossMaxLifeCount);

	ApplyBossLifeWidgets();
}

void UJunCombatHUDWidget::SetPlayerPosture(float CurrentPosture, float MaxPosture)
{
	PlayerPosturePercent = MaxPosture > 0.f
		? FMath::Clamp(CurrentPosture / MaxPosture, 0.f, 1.f)
		: 0.f;

	if (PlayerPosturePercent > KINDA_SMALL_NUMBER)
	{
		bPlayerPostureBreakHidePresentationActive = false;
		PlayerPostureFrameOpacity = 1.f;
		if (PlayerPosture_Frame)
		{
			PlayerPosture_Frame->SetRenderOpacity(PlayerPostureFrameOpacity);
			PlayerPosture_Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	ApplyPostureBars();
}

void UJunCombatHUDWidget::ResetPlayerPostureVisibilityState()
{
	PlayerPosturePercent = 0.f;
	PlayerZeroPostureElapsedTime = 0.f;
	bPlayerPostureEverShown = false;
	bPlayerPostureBreakHidePresentationActive = false;
	PlayerPostureFrameOpacity = 1.f;
	if (PlayerPosture_Frame)
	{
		PlayerPosture_Frame->SetRenderOpacity(PlayerPostureFrameOpacity);
		PlayerPosture_Frame->SetVisibility(ESlateVisibility::Hidden);
	}
	ApplyPostureBars();
	ApplyPlayerPostureVisibility(false);
}

void UJunCombatHUDWidget::StartPlayerPostureBreakHidePresentation()
{
	PlayerPosturePercent = 0.f;
	PlayerZeroPostureElapsedTime = PostureHideDelay;
	bPlayerPostureEverShown = true;
	bPlayerPostureBreakHidePresentationActive = true;
	PlayerPostureFrameOpacity = 1.f;
	ApplyPostureBars();
	if (PlayerPostureRoot)
	{
		PlayerPostureRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (PlayerPosture_Frame)
	{
		PlayerPosture_Frame->SetRenderOpacity(PlayerPostureFrameOpacity);
		PlayerPosture_Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	ApplyPlayerPostureFillVisibility(false);
}

void UJunCombatHUDWidget::SetBossPosture(float CurrentPosture, float MaxPosture)
{
	const float PreviousBossPosturePercent = BossPosturePercent;
	BossPosturePercent = MaxPosture > 0.f
		? FMath::Clamp(CurrentPosture / MaxPosture, 0.f, 1.f)
		: 0.f;

	if (PreviousBossPosturePercent < 1.f && BossPosturePercent >= 1.f)
	{
		PlayBossPostureBreakGlow();
	}

	ApplyPostureBars();
}

void UJunCombatHUDWidget::HideBossPostureImmediately()
{
	BossPosturePercent = 0.f;
	BossZeroPostureElapsedTime = PostureHideDelay;
	bBossPostureEverShown = false;
	ApplyPostureBars();
	ApplyBossPostureVisibility(false);
}

void UJunCombatHUDWidget::PlayPlayerPostureBreakGlow()
{
	StartRedGlowEffect(Player_RedGlow_Root, PlayerRedGlowElapsedTime);
}

void UJunCombatHUDWidget::PlayBossPostureBreakGlow()
{
	StartRedGlowEffect(Boss_RedGlow_Root, BossRedGlowElapsedTime);
}

void UJunCombatHUDWidget::ShowBossClearUI()
{
	bBossClearUIActive = true;
	BossClearHoldRemainTime = BossClearHoldDuration;
	TargetBossClearOpacity = 1.f;
	SetBossHealthVisible(false);
	if (BossPostureRoot)
	{
		BossPostureRoot->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Boss_Clear)
	{
		Boss_Clear->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunCombatHUDWidget::ShowFakeDeathUI()
{
	SetDeathRootVisible(true);
	TargetDimBlackOpacity = FakeDeathDimBlackOpacity;
	TargetFakeDeathOpacity = 1.f;
	TargetRealDeathOpacity = 0.f;
	TargetFullBlackOpacity = 0.f;

	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UJunCombatHUDWidget::HideFakeDeathUI()
{
	TargetFakeDeathOpacity = 0.f;
	TargetDimBlackOpacity = 0.f;
	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::ShowRealDeathUI()
{
	SetDeathRootVisible(true);
	TargetDimBlackOpacity = RealDeathDimBlackOpacity;
	TargetFakeDeathOpacity = 0.f;
	TargetRealDeathOpacity = 1.f;
	TargetFullBlackOpacity = 0.f;
	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::StartFullBlackFadeIn()
{
	SetDeathRootVisible(true);
	bRespawnFullBlackFadeOutActive = false;
	TargetFullBlackOpacity = 1.f;
}

void UJunCombatHUDWidget::StartRespawnFadeOut()
{
	SetDeathRootVisible(true);
	bRespawnFullBlackFadeOutActive = true;

	DimBlackOpacity = 0.f;
	FakeDeathOpacity = 0.f;
	RealDeathOpacity = 0.f;
	TargetDimBlackOpacity = 0.f;
	TargetFakeDeathOpacity = 0.f;
	TargetRealDeathOpacity = 0.f;
	TargetFullBlackOpacity = 0.f;

	ApplyWidgetOpacity(DimBlack, 0.f);
	ApplyWidgetOpacity(Fake_Death, 0.f);
	ApplyWidgetOpacity(Real_Death, 0.f);
	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::HideDeathUI()
{
	bRespawnFullBlackFadeOutActive = false;
	DimBlackOpacity = 0.f;
	FullBlackOpacity = 0.f;
	FakeDeathOpacity = 0.f;
	RealDeathOpacity = 0.f;
	TargetDimBlackOpacity = 0.f;
	TargetFullBlackOpacity = 0.f;
	TargetFakeDeathOpacity = 0.f;
	TargetRealDeathOpacity = 0.f;
	ApplyWidgetOpacity(DimBlack, 0.f);
	ApplyWidgetOpacity(FullBlack, 0.f);
	ApplyWidgetOpacity(Fake_Death, 0.f);
	ApplyWidgetOpacity(Real_Death, 0.f);
	if (Die_Select)
	{
		Die_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Resurrect_Select)
	{
		Resurrect_Select->SetVisibility(ESlateVisibility::Hidden);
	}
	SetDeathRootVisible(false);
}

bool UJunCombatHUDWidget::IsFullBlackOpaque() const
{
	return FullBlackOpacity >= 0.99f;
}

void UJunCombatHUDWidget::SetBossHealthVisible(bool bVisible)
{
	if (bBossClearUIActive)
	{
		bVisible = false;
	}

	if (bBossHealthVisible == bVisible)
	{
		ApplyBossHealthVisibility();
		OnBossHealthVisibilityChanged(bBossHealthVisible);
		return;
	}

	bBossHealthVisible = bVisible;
	ApplyBossHealthVisibility();
	OnBossHealthVisibilityChanged(bBossHealthVisible);
}

ESlateVisibility UJunCombatHUDWidget::GetBossHealthSlateVisibility() const
{
	return bBossHealthVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
}

void UJunCombatHUDWidget::UpdateDelayedHealthBars(float DeltaTime)
{
	UpdateDelayedHealthBar(
		DeltaTime,
		PlayerHealthPercent,
		PlayerDelayedHealthPercent,
		PlayerDelayedHealthDelayRemainTime,
		&UJunCombatHUDWidget::OnPlayerDelayedHealthChanged);

	UpdateDelayedHealthBar(
		DeltaTime,
		BossHealthPercent,
		BossDelayedHealthPercent,
		BossDelayedHealthDelayRemainTime,
		&UJunCombatHUDWidget::OnBossDelayedHealthChanged);
}

void UJunCombatHUDWidget::UpdateBossHealthFill(float DeltaTime)
{
	if (BossHealthPercent >= BossTargetHealthPercent)
	{
		return;
	}

	const float PreviousHealthPercent = BossHealthPercent;
	BossHealthPercent = BossHealthFillInterpSpeed > 0.f
		? FMath::FInterpTo(BossHealthPercent, BossTargetHealthPercent, DeltaTime, BossHealthFillInterpSpeed)
		: BossTargetHealthPercent;

	if (FMath::IsNearlyEqual(BossHealthPercent, BossTargetHealthPercent, 0.001f))
	{
		BossHealthPercent = BossTargetHealthPercent;
	}

	if (!FMath::IsNearlyEqual(PreviousHealthPercent, BossHealthPercent, 0.0001f))
	{
		if (BossDelayedHealthPercent < BossHealthPercent)
		{
			BossDelayedHealthPercent = BossHealthPercent;
			OnBossDelayedHealthChanged(BossDelayedHealthPercent);
		}

		ApplyBossHealthBars();
	}
}

void UJunCombatHUDWidget::UpdateRedGlowEffects(float DeltaTime)
{
	UpdateRedGlowEffect(DeltaTime, Player_RedGlow_Root, PlayerRedGlowElapsedTime);
	UpdateRedGlowEffect(DeltaTime, Boss_RedGlow_Root, BossRedGlowElapsedTime);
}

void UJunCombatHUDWidget::UpdateRedGlowEffect(float DeltaTime, UWidget* RootWidget, float& ElapsedTime)
{
	if (!RootWidget || ElapsedTime < 0.f)
	{
		return;
	}

	ElapsedTime += DeltaTime;
	const float Alpha = RedGlowDuration > 0.f
		? FMath::Clamp(ElapsedTime / RedGlowDuration, 0.f, 1.f)
		: 1.f;
	const float EaseAlpha = 1.f - FMath::Square(1.f - Alpha);
	const float ScaleX = FMath::Lerp(RedGlowStartScaleX, RedGlowEndScaleX, EaseAlpha);
	const float Opacity = FMath::Pow(1.f - Alpha, 0.65f);

	RootWidget->SetRenderScale(FVector2D(ScaleX, RedGlowScaleY));
	RootWidget->SetRenderOpacity(Opacity);
	RootWidget->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	if (Alpha >= 1.f)
	{
		ElapsedTime = -1.f;
		RootWidget->SetRenderOpacity(0.f);
		RootWidget->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
		RootWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::StartRedGlowEffect(UWidget* RootWidget, float& ElapsedTime)
{
	if (!RootWidget)
	{
		return;
	}

	ElapsedTime = 0.f;
	RootWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	RootWidget->SetRenderScale(FVector2D(RedGlowStartScaleX, RedGlowScaleY));
	RootWidget->SetRenderOpacity(1.f);
	RootWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UJunCombatHUDWidget::UpdateDeathUI(float DeltaTime)
{
	const auto InterpOpacity = [this, DeltaTime](float Current, float Target)
	{
		return DeathUIFadeSpeed > 0.f
			? FMath::FInterpTo(Current, Target, DeltaTime, DeathUIFadeSpeed)
			: Target;
	};

	DimBlackOpacity = InterpOpacity(DimBlackOpacity, TargetDimBlackOpacity);
	const float FullBlackFadeSpeed = bRespawnFullBlackFadeOutActive
		? RespawnFullBlackFadeOutSpeed
		: DeathUIFadeSpeed;
	FullBlackOpacity = FullBlackFadeSpeed > 0.f
		? FMath::FInterpTo(FullBlackOpacity, TargetFullBlackOpacity, DeltaTime, FullBlackFadeSpeed)
		: TargetFullBlackOpacity;
	FakeDeathOpacity = InterpOpacity(FakeDeathOpacity, TargetFakeDeathOpacity);
	RealDeathOpacity = InterpOpacity(RealDeathOpacity, TargetRealDeathOpacity);

	if (FMath::IsNearlyEqual(DimBlackOpacity, TargetDimBlackOpacity, 0.005f))
	{
		DimBlackOpacity = TargetDimBlackOpacity;
	}
	if (FMath::IsNearlyEqual(FullBlackOpacity, TargetFullBlackOpacity, 0.005f))
	{
		FullBlackOpacity = TargetFullBlackOpacity;
		if (FullBlackOpacity <= 0.f)
		{
			bRespawnFullBlackFadeOutActive = false;
		}
	}
	if (FMath::IsNearlyEqual(FakeDeathOpacity, TargetFakeDeathOpacity, 0.005f))
	{
		FakeDeathOpacity = TargetFakeDeathOpacity;
	}
	if (FMath::IsNearlyEqual(RealDeathOpacity, TargetRealDeathOpacity, 0.005f))
	{
		RealDeathOpacity = TargetRealDeathOpacity;
	}

	ApplyWidgetOpacity(DimBlack, DimBlackOpacity);
	ApplyWidgetOpacity(FullBlack, FullBlackOpacity);
	ApplyWidgetOpacity(Fake_Death, FakeDeathOpacity);
	ApplyWidgetOpacity(Real_Death, RealDeathOpacity);

	const bool bAnyDeathUIVisible =
		DimBlackOpacity > 0.f ||
		FullBlackOpacity > 0.f ||
		FakeDeathOpacity > 0.f ||
		RealDeathOpacity > 0.f ||
		TargetDimBlackOpacity > 0.f ||
		TargetFullBlackOpacity > 0.f ||
		TargetFakeDeathOpacity > 0.f ||
		TargetRealDeathOpacity > 0.f;
	SetDeathRootVisible(bAnyDeathUIVisible);
}

void UJunCombatHUDWidget::UpdateDialogueUI(float DeltaTime)
{
	DialogueBackgroundOpacity = DialogueBackgroundFadeSpeed > 0.f
		? FMath::FInterpTo(DialogueBackgroundOpacity, TargetDialogueBackgroundOpacity, DeltaTime, DialogueBackgroundFadeSpeed)
		: TargetDialogueBackgroundOpacity;

	if (FMath::IsNearlyEqual(DialogueBackgroundOpacity, TargetDialogueBackgroundOpacity, 0.005f))
	{
		DialogueBackgroundOpacity = TargetDialogueBackgroundOpacity;
	}

	if (Dialog_Background)
	{
		ApplyWidgetOpacity(Dialog_Background, DialogueBackgroundOpacity);
	}

	ApplyDialogueVisibility();
}

void UJunCombatHUDWidget::UpdateTutorialDimBlackUI(float DeltaTime)
{
	TutorialDimBlackOpacity = TutorialDimBlackFadeSpeed > 0.f
		? FMath::FInterpTo(TutorialDimBlackOpacity, TargetTutorialDimBlackOpacity, DeltaTime, TutorialDimBlackFadeSpeed)
		: TargetTutorialDimBlackOpacity;

	if (FMath::IsNearlyEqual(TutorialDimBlackOpacity, TargetTutorialDimBlackOpacity, 0.005f))
	{
		TutorialDimBlackOpacity = TargetTutorialDimBlackOpacity;
	}

	ApplyWidgetOpacity(Tutorial_DimBlack, TutorialDimBlackOpacity);
}

void UJunCombatHUDWidget::UpdateBossClearUI(float DeltaTime)
{
	const bool bFadingOut = TargetBossClearOpacity < BossClearOpacity;
	const float InterpSpeed = bFadingOut ? BossClearFadeOutSpeed : BossClearFadeInSpeed;
	BossClearOpacity = InterpSpeed > 0.f
		? FMath::FInterpTo(BossClearOpacity, TargetBossClearOpacity, DeltaTime, InterpSpeed)
		: TargetBossClearOpacity;

	if (FMath::IsNearlyEqual(BossClearOpacity, TargetBossClearOpacity, 0.005f))
	{
		BossClearOpacity = TargetBossClearOpacity;
	}

	if (BossClearHoldRemainTime > 0.f && BossClearOpacity >= 0.99f)
	{
		BossClearHoldRemainTime = FMath::Max(0.f, BossClearHoldRemainTime - DeltaTime);
		if (BossClearHoldRemainTime <= 0.f)
		{
			TargetBossClearOpacity = 0.f;
		}
	}

	ApplyWidgetOpacity(Boss_Clear, BossClearOpacity);
}

void UJunCombatHUDWidget::ApplyWidgetOpacity(UWidget* Widget, float Opacity, bool bHideWhenZero) const
{
	if (!Widget)
	{
		return;
	}

	Widget->SetRenderOpacity(Opacity);
	Widget->SetVisibility(!bHideWhenZero || Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UJunCombatHUDWidget::SetDeathRootVisible(bool bVisible)
{
	if (Death_Root)
	{
		Death_Root->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::ApplyDialogueVisibility()
{
	const bool bHasText = !CurrentDialogueText.IsEmpty();
	const bool bBackgroundVisible = DialogueBackgroundOpacity > 0.f || TargetDialogueBackgroundOpacity > 0.f;
	const bool bVisible = bHasText || bBackgroundVisible;
	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;

	if (Dialog_Root)
	{
		Dialog_Root->SetVisibility(TargetVisibility);
	}

	if (Dialog_Background)
	{
		Dialog_Background->SetVisibility(bBackgroundVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Dialog_Text_1)
	{
		Dialog_Text_1->SetVisibility(bHasText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Dialog_Text_2)
	{
		Dialog_Text_2->SetVisibility(!CurrentDialogueSpeakerName.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Key_E)
	{
		Key_E->SetVisibility(bHasText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Next_Text)
	{
		Next_Text->SetVisibility(bHasText ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::ApplyDialogueText()
{
	if (Dialog_Text_1)
	{
		Dialog_Text_1->SetText(CurrentDialogueText);
	}

	if (Dialog_Text_2)
	{
		Dialog_Text_2->SetText(CurrentDialogueSpeakerName);
	}
}

void UJunCombatHUDWidget::UpdateDelayedHealthBar(
	float DeltaTime,
	float CurrentHealthPercent,
	float& DelayedHealthPercent,
	float& DelayRemainTime,
	void (UJunCombatHUDWidget::*OnDelayedHealthChanged)(float))
{
	if (DelayedHealthPercent <= CurrentHealthPercent)
	{
		return;
	}

	if (DelayRemainTime > 0.f)
	{
		DelayRemainTime = FMath::Max(0.f, DelayRemainTime - DeltaTime);
		return;
	}

	const float PreviousDelayedHealthPercent = DelayedHealthPercent;
	DelayedHealthPercent = FMath::FInterpTo(
		DelayedHealthPercent,
		CurrentHealthPercent,
		DeltaTime,
		DelayedHealthInterpSpeed
	);

	if (FMath::IsNearlyEqual(DelayedHealthPercent, CurrentHealthPercent, 0.001f))
	{
		DelayedHealthPercent = CurrentHealthPercent;
	}

	if (!FMath::IsNearlyEqual(PreviousDelayedHealthPercent, DelayedHealthPercent, 0.0001f))
	{
		(this->*OnDelayedHealthChanged)(DelayedHealthPercent);
		ApplyPlayerHealthBars();
		ApplyBossHealthBars();
	}
}

void UJunCombatHUDWidget::ApplyPlayerHealthBars()
{
	if (PlayerHealthBar)
	{
		PlayerHealthBar->SetPercent(PlayerHealthPercent);
	}

	if (PlayerDelayedHealthBar)
	{
		PlayerDelayedHealthBar->SetPercent(PlayerDelayedHealthPercent);
	}
}

void UJunCombatHUDWidget::ApplyBossHealthBars()
{
	if (BossHealthBar)
	{
		BossHealthBar->SetPercent(BossHealthPercent);
	}

	if (BossDelayedHealthBar)
	{
		BossDelayedHealthBar->SetPercent(BossDelayedHealthPercent);
	}
}

void UJunCombatHUDWidget::ApplyPostureBars()
{
	ApplyPostureFill(BossFillCenter, BossPosturePercent, BossPostureFillMaxWidth);
	ApplyPostureFill(PlayerFillCenter, PlayerPosturePercent, PlayerPostureFillMaxWidth);
	ApplyPostureTint(BossCenter_Image, BossPosturePercent);
	ApplyPostureTint(BossLeftCap, BossPosturePercent);
	ApplyPostureTint(BossRightCap, BossPosturePercent);
	ApplyPostureTint(PlayerCenter_Image, PlayerPosturePercent);
	ApplyPostureTint(PlayerLeftCap, PlayerPosturePercent);
	ApplyPostureTint(PlayerRightCap, PlayerPosturePercent);
}

void UJunCombatHUDWidget::UpdatePostureVisibility(float DeltaTime)
{
	const bool bPlayerHasPosture = PlayerPosturePercent > KINDA_SMALL_NUMBER;
	const bool bBossHasPosture = BossPosturePercent > KINDA_SMALL_NUMBER;

	if (bPlayerHasPosture)
	{
		bPlayerPostureEverShown = true;
	}
	if (bBossHasPosture)
	{
		bBossPostureEverShown = true;
	}

	PlayerZeroPostureElapsedTime = bPlayerHasPosture
		? 0.f
		: PlayerZeroPostureElapsedTime + DeltaTime;
	BossZeroPostureElapsedTime = bBossHasPosture
		? 0.f
		: BossZeroPostureElapsedTime + DeltaTime;

	if (bPlayerPostureBreakHidePresentationActive)
	{
		PlayerPostureFrameOpacity = PlayerPostureBreakFrameFadeOutSpeed > 0.f
			? FMath::FInterpTo(PlayerPostureFrameOpacity, 0.f, DeltaTime, PlayerPostureBreakFrameFadeOutSpeed)
			: 0.f;

		if (PlayerPostureRoot)
		{
			PlayerPostureRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (PlayerPosture_Frame)
		{
			PlayerPosture_Frame->SetRenderOpacity(PlayerPostureFrameOpacity);
			PlayerPosture_Frame->SetVisibility(PlayerPostureFrameOpacity > 0.01f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		}

		ApplyPlayerPostureFillVisibility(false);
		if (PlayerPostureFrameOpacity <= 0.01f)
		{
			bPlayerPostureBreakHidePresentationActive = false;
			PlayerPostureFrameOpacity = 1.f;
			if (PlayerPosture_Frame)
			{
				PlayerPosture_Frame->SetRenderOpacity(PlayerPostureFrameOpacity);
			}
			ApplyPlayerPostureVisibility(false);
		}
	}
	else
	{
		ApplyPlayerPostureVisibility(
			bPlayerPostureEverShown &&
			(bPlayerHasPosture || PlayerZeroPostureElapsedTime < PostureHideDelay));
	}

	ApplyBossPostureVisibility(
		bBossPostureEverShown &&
		bBossHealthVisible &&
		!bBossClearUIActive &&
		(bBossHasPosture || BossZeroPostureElapsedTime < PostureHideDelay));
}

void UJunCombatHUDWidget::ApplyPostureFill(USizeBox* FillCenter, float Percent, float MaxWidth) const
{
	if (!FillCenter)
	{
		return;
	}

	FillCenter->SetWidthOverride(FMath::Max(0.f, MaxWidth * FMath::Clamp(Percent, 0.f, 1.f)));
	FillCenter->SetHeightOverride(PostureFillHeight);
}

void UJunCombatHUDWidget::ApplyPostureTint(UImage* FillImage, float Percent) const
{
	if (!FillImage)
	{
		return;
	}

	const FLinearColor TargetTint = Percent >= PostureWarningTintThreshold
		? PostureWarningTint
		: PostureNormalTint;
	FillImage->SetColorAndOpacity(TargetTint);
}

void UJunCombatHUDWidget::ApplyPlayerPostureFillVisibility(bool bVisible)
{
	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (PlayerFillCenter)
	{
		PlayerFillCenter->SetVisibility(TargetVisibility);
	}
	if (PlayerCenter_Image)
	{
		PlayerCenter_Image->SetVisibility(TargetVisibility);
	}
	if (PlayerLeftCap)
	{
		PlayerLeftCap->SetVisibility(TargetVisibility);
	}
	if (PlayerRightCap)
	{
		PlayerRightCap->SetVisibility(TargetVisibility);
	}
}

void UJunCombatHUDWidget::ApplyBossPostureFillVisibility(bool bVisible)
{
	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (BossFillCenter)
	{
		BossFillCenter->SetVisibility(TargetVisibility);
	}
	if (BossCenter_Image)
	{
		BossCenter_Image->SetVisibility(TargetVisibility);
	}
	if (BossLeftCap)
	{
		BossLeftCap->SetVisibility(TargetVisibility);
	}
	if (BossRightCap)
	{
		BossRightCap->SetVisibility(TargetVisibility);
	}
}

void UJunCombatHUDWidget::ApplyPlayerPostureVisibility(bool bVisible)
{
	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (PlayerPostureRoot)
	{
		PlayerPostureRoot->SetVisibility(TargetVisibility);
	}

	if (PlayerPosture_Frame)
	{
		PlayerPosture_Frame->SetRenderOpacity(1.f);
		PlayerPosture_Frame->SetVisibility(TargetVisibility);
	}

	ApplyPlayerPostureFillVisibility(bVisible);
}

void UJunCombatHUDWidget::ApplyBossPostureVisibility(bool bVisible)
{
	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (BossPostureRoot)
	{
		BossPostureRoot->SetVisibility(TargetVisibility);
	}

	ApplyBossPostureFillVisibility(bVisible);
}

void UJunCombatHUDWidget::ApplyBossHealthVisibility()
{
	if (BossHealthRoot)
	{
		BossHealthRoot->SetVisibility(GetBossHealthSlateVisibility());
	}

	if (BossPostureRoot)
	{
		const bool bShouldShowBossPosture =
			bBossPostureEverShown &&
			bBossHealthVisible &&
			!bBossClearUIActive &&
			(BossPosturePercent > KINDA_SMALL_NUMBER || BossZeroPostureElapsedTime < PostureHideDelay);
		BossPostureRoot->SetVisibility(bShouldShowBossPosture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::ApplyBossLifeWidgets()
{
	ApplyBossLifeWidget(1, Life_1_Root, Life_1_Gray, Life_1_Red);
	ApplyBossLifeWidget(2, Life_2_Root, Life_2_Gray, Life_2_Red);
	ApplyBossLifeWidget(3, Life_3_Root, Life_3_Gray, Life_3_Red);
}

void UJunCombatHUDWidget::ApplyBossLifeWidget(int32 LifeIndex, UWidget* RootWidget, UWidget* GrayWidget, UWidget* RedWidget) const
{
	const bool bActiveLifeSlot = bBossHealthVisible && LifeIndex <= BossMaxLifeCount;
	const bool bLifeRemaining = LifeIndex <= BossCurrentLifeCount;

	if (RootWidget)
	{
		RootWidget->SetVisibility(bActiveLifeSlot ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (RedWidget)
	{
		RedWidget->SetVisibility(bActiveLifeSlot && bLifeRemaining ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (GrayWidget)
	{
		GrayWidget->SetVisibility(bActiveLifeSlot && !bLifeRemaining ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UJunCombatHUDWidget::ApplyPotionWidgets()
{
	if (Potion_Root)
	{
		Potion_Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const bool bHasPotion = PotionCount > 0;
	if (Potion_Image)
	{
		Potion_Image->SetVisibility(bHasPotion ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (Empty_Potion_Image)
	{
		Empty_Potion_Image->SetVisibility(bHasPotion ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	}

	if (Potion_Count)
	{
		Potion_Count->SetText(FText::AsNumber(PotionCount));
	}
}

void UJunCombatHUDWidget::ApplyControlsVisibility()
{
	if (Controls_Root)
	{
		Controls_Root->SetVisibility(bControlsVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}
