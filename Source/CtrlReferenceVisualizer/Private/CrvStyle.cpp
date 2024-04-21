#include "CrvStyle.h"

#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/StyleColors.h"

TSharedPtr<FCrvStyle> FCrvStyle::StyleSet = nullptr;

TSharedPtr<FCrvStyle> FCrvStyle::Get()
{
	ensure(StyleSet.IsValid());
	return StyleSet;
}

FCrvStyle::FCrvStyle()
	: FSlateStyleSet(FCrvStyle::GetStyleSetName())
{
}

void FCrvStyle::Startup()
{
	if (StyleSet.IsValid()) { return; } // only init once
	StyleSet = MakeShared<FCrvStyle>();
	StyleSet->Initialize();
}

void FCrvStyle::Register()
{
	if (!StyleSet.IsValid()) { return; }
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FCrvStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

const FName& FCrvStyle::GetStyleSetName() const
{
	static FName CtrlStyleName(TEXT("CrvStyle"));
	return CtrlStyleName;
}

void FCrvStyle::Initialize()
{
	const FVector2D Icon128x128(128.0f, 128.0f);
	const FVector2D Icon16x16(16.f, 16.f);
	const FVector2D Icon32x32(32.f, 32.f);

	SetContentRoot(FPaths::ProjectPluginsDir() / TEXT("CtrlReferenceVisualizer/Resources"));
	Set("Ctrl.TabIcon", new FSlateImageBrush(GetContentRootDir() / TEXT("Icon128.png"), Icon128x128));
	Set("Ctrl.PlacementModeIcon", new FSlateImageBrush(GetContentRootDir() / TEXT("Icon16.png"), Icon16x16));
	// use outliner style to get colors for selection + unfocused selection + hover
	auto RowStyle = FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("SceneOutliner.TableViewRow");
	// add alternating row color
	FLinearColor AltColor = FStyleColors::Panel.GetSpecifiedColor().CopyWithNewOpacity(0.2f);
	Set(
		"Ctrl.TreeView.TableRow",
		FTableRowStyle(RowStyle)
		.SetEvenRowBackgroundBrush(FSlateColorBrush(FStyleColors::Recessed))
		.SetOddRowBackgroundBrush(FSlateColorBrush(FLinearColor(AltColor)))
	);
}
