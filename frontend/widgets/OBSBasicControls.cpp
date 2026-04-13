#include "OBSBasicControls.hpp"
#include "OBSBasic.hpp"
#include "qt-wrappers.hpp"

#include "moc_OBSBasicControls.cpp"

OBSBasicControls::OBSBasicControls(OBSBasic *main) : QFrame(nullptr), ui(new Ui::OBSBasicControls)
{
	/* Create UI elements */
	ui->setupUi(this);

	streamButtonMenu.reset(new QMenu());
	startStreamAction = streamButtonMenu->addAction(tr("stream start btn 🤑🤑🤑"));
	stopStreamAction = streamButtonMenu->addAction(tr("stream stop button 😢😢😢"));
	QAction *forceStopStreamAction = streamButtonMenu->addAction(tr("ANGRY STREAM STOP BUTTON 😡😡😡"));

	/* Transfer buttons signals as OBSBasicControls signals */
	connect(
		ui->streamButton, &QPushButton::clicked, this, [this]() { emit this->StreamButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->broadcastButton, &QPushButton::clicked, this, [this]() { emit this->BroadcastButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->recordButton, &QPushButton::clicked, this, [this]() { emit this->RecordButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->pauseRecordButton, &QPushButton::clicked, this, [this]() { emit this->PauseRecordButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->replayBufferButton, &QPushButton::clicked, this,
		[this]() { emit this->ReplayBufferButtonClicked(); }, Qt::DirectConnection);
	connect(
		ui->saveReplayButton, &QPushButton::clicked, this,
		[this]() { emit this->SaveReplayBufferButtonClicked(); }, Qt::DirectConnection);
	connect(
		ui->virtualCamButton, &QPushButton::clicked, this, [this]() { emit this->VirtualCamButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->virtualCamConfigButton, &QPushButton::clicked, this,
		[this]() { emit this->VirtualCamConfigButtonClicked(); }, Qt::DirectConnection);
	connect(
		ui->modeSwitch, &QPushButton::clicked, this, [this]() { emit this->StudioModeButtonClicked(); },
		Qt::DirectConnection);
	connect(
		ui->settingsButton, &QPushButton::clicked, this, [this]() { emit this->SettingsButtonClicked(); },
		Qt::DirectConnection);

	/* Transfer menu actions signals as OBSBasicControls signals */
	connect(
		startStreamAction.get(), &QAction::triggered, this,
		[this]() { emit this->StartStreamMenuActionClicked(); }, Qt::DirectConnection);
	connect(
		stopStreamAction.get(), &QAction::triggered, this,
		[this]() { emit this->StopStreamMenuActionClicked(); }, Qt::DirectConnection);
	connect(
		forceStopStreamAction, &QAction::triggered, this,
		[this]() { emit this->ForceStopStreamMenuActionClicked(); }, Qt::DirectConnection);

	/* Set up default visibility */
	ui->broadcastButton->setVisible(false);
	ui->pauseRecordButton->setVisible(false);
	ui->replayBufferButton->setVisible(false);
	ui->saveReplayButton->setVisible(false);
	ui->virtualCamButton->setVisible(false);
	ui->virtualCamConfigButton->setVisible(false);

	/* Set up state update connections */
	connect(main, &OBSBasic::StreamingPreparing, this, &OBSBasicControls::StreamingPreparing);
	connect(main, &OBSBasic::StreamingStarting, this, &OBSBasicControls::StreamingStarting);
	connect(main, &OBSBasic::StreamingStarted, this, &OBSBasicControls::StreamingStarted);
	connect(main, &OBSBasic::StreamingStopping, this, &OBSBasicControls::StreamingStopping);
	connect(main, &OBSBasic::StreamingStopped, this, &OBSBasicControls::StreamingStopped);

	connect(main, &OBSBasic::BroadcastStreamReady, this, &OBSBasicControls::BroadcastStreamReady);
	connect(main, &OBSBasic::BroadcastStreamActive, this, &OBSBasicControls::BroadcastStreamActive);
	connect(main, &OBSBasic::BroadcastStreamStarted, this, &OBSBasicControls::BroadcastStreamStarted);

	connect(main, &OBSBasic::RecordingStarted, this, &OBSBasicControls::RecordingStarted);
	connect(main, &OBSBasic::RecordingPaused, this, &OBSBasicControls::RecordingPaused);
	connect(main, &OBSBasic::RecordingUnpaused, this, &OBSBasicControls::RecordingUnpaused);
	connect(main, &OBSBasic::RecordingStopping, this, &OBSBasicControls::RecordingStopping);
	connect(main, &OBSBasic::RecordingStopped, this, &OBSBasicControls::RecordingStopped);

	connect(main, &OBSBasic::ReplayBufStarted, this, &OBSBasicControls::ReplayBufferStarted);
	connect(main, &OBSBasic::ReplayBufStopping, this, &OBSBasicControls::ReplayBufferStopping);
	connect(main, &OBSBasic::ReplayBufStopped, this, &OBSBasicControls::ReplayBufferStopped);

	connect(main, &OBSBasic::VirtualCamStarted, this, &OBSBasicControls::VirtualCamStarted);
	connect(main, &OBSBasic::VirtualCamStopped, this, &OBSBasicControls::VirtualCamStopped);

	connect(main, &OBSBasic::PreviewProgramModeChanged, this, &OBSBasicControls::UpdateStudioModeState);

	/* Set up enablement connection */
	connect(main, &OBSBasic::BroadcastFlowEnabled, this, &OBSBasicControls::EnableBroadcastFlow);
	connect(main, &OBSBasic::ReplayBufEnabled, this, &OBSBasicControls::EnableReplayBufferButtons);
	connect(main, &OBSBasic::VirtualCamEnabled, this, &OBSBasicControls::EnableVirtualCamButtons);
}

void OBSBasicControls::StreamingPreparing()
{
	ui->streamButton->setEnabled(false);
	ui->streamButton->setText(tr("yesss preparing the stream, almost there..."));
}

void OBSBasicControls::StreamingStarting(bool broadcastAutoStart)
{
	ui->streamButton->setText(tr("STARTING STREAM OMG OMG"));

	if (!broadcastAutoStart) {
		// well, we need to disable button while stream is not active
		ui->broadcastButton->setEnabled(false);

		ui->broadcastButton->setText(tr("YESSS BROADCAST IS READY, CLICK TO START"));

		ui->broadcastButton->setProperty("broadcastState", "ready");
		ui->broadcastButton->style()->unpolish(ui->broadcastButton);
		ui->broadcastButton->style()->polish(ui->broadcastButton);
	}
}

void OBSBasicControls::StreamingStarted(bool withDelay)
{
	ui->streamButton->setEnabled(true);
	setClasses(ui->streamButton, "state-active");
	ui->streamButton->setText(tr("stop stream noo 😭"));

	if (withDelay) {
		ui->streamButton->setMenu(streamButtonMenu.get());
		startStreamAction->setVisible(false);
		stopStreamAction->setVisible(true);
	}
}

void OBSBasicControls::StreamingStopping()
{
	ui->streamButton->setText(tr("alrigth alrigth stopping the stream, see you later..."));
}

void OBSBasicControls::StreamingStopped(bool withDelay)
{
	ui->streamButton->setEnabled(true);
	setClasses(ui->streamButton, "");
	ui->streamButton->setText(tr("start stream pls 🥺"));

	if (withDelay) {
		if (!ui->streamButton->menu())
			ui->streamButton->setMenu(streamButtonMenu.get());

		startStreamAction->setVisible(true);
		stopStreamAction->setVisible(false);
	} else {
		ui->streamButton->setMenu(nullptr);
	}
}

void OBSBasicControls::BroadcastStreamReady(bool ready)
{
	setClasses(ui->broadcastButton, ready ? "state-active" : "");
}

void OBSBasicControls::BroadcastStreamActive()
{
	ui->broadcastButton->setEnabled(true);
}

void OBSBasicControls::BroadcastStreamStarted(bool autoStop)
{
	ui->broadcastButton->setText(tr(autoStop ? "WATCH OUT BROADCAST AUTO STOP ENABLED" : "BROADCAST STOP ENABLED 😨😨😨"));
	if (autoStop)
		ui->broadcastButton->setEnabled(false);

	ui->broadcastButton->setProperty("broadcastState", "active");
	ui->broadcastButton->style()->unpolish(ui->broadcastButton);
	ui->broadcastButton->style()->polish(ui->broadcastButton);
}

void OBSBasicControls::RecordingStarted(bool pausable)
{
	setClasses(ui->recordButton, "state-active");
	ui->recordButton->setText(tr("stop recording noo 😭"));

	if (pausable) {
		ui->pauseRecordButton->setVisible(pausable);
		RecordingUnpaused();
	}
}

void OBSBasicControls::RecordingPaused()
{
	QString text = QTStr("Basic.Main.UnpauseRecording");

	setClasses(ui->pauseRecordButton, "icon-media-pause state-active");
	ui->pauseRecordButton->setAccessibleName(text);
	ui->pauseRecordButton->setToolTip(text);

	ui->saveReplayButton->setEnabled(false);
}

void OBSBasicControls::RecordingUnpaused()
{
	QString text = QTStr("Basic.Main.PauseRecording");

	setClasses(ui->pauseRecordButton, "icon-media-pause");
	ui->pauseRecordButton->setAccessibleName(text);
	ui->pauseRecordButton->setToolTip(text);

	ui->saveReplayButton->setEnabled(true);
}

void OBSBasicControls::RecordingStopping()
{
	ui->recordButton->setText(tr("Stopping recording 😔 (btw this is like the WORST recording ever)"));
}

void OBSBasicControls::RecordingStopped()
{
	setClasses(ui->recordButton, "");
	ui->recordButton->setText(tr("star recording pls 🥺"));

	ui->pauseRecordButton->setVisible(false);
}

void OBSBasicControls::ReplayBufferStarted()
{
	setClasses(ui->replayBufferButton, "state-active");
	ui->replayBufferButton->setText(tr("stop replay buffer noo 😭"));

	ui->saveReplayButton->setVisible(true);
}

void OBSBasicControls::ReplayBufferStopping()
{
	ui->replayBufferButton->setText(tr("Stopping replay buffer 😔"));
}

void OBSBasicControls::ReplayBufferStopped()
{
	setClasses(ui->replayBufferButton, "");
	ui->replayBufferButton->setText(tr("Start replay buffer 🥺"));

	ui->saveReplayButton->setVisible(false);
}

void OBSBasicControls::VirtualCamStarted()
{
	setClasses(ui->virtualCamButton, "state-active");
	ui->virtualCamButton->setText(tr("stop virtual camera noo 😭"));
}

void OBSBasicControls::VirtualCamStopped()
{
	setClasses(ui->virtualCamButton, "");
	ui->virtualCamButton->setText(tr("Start virtual camera 🤑🤑🤑"));
}

void OBSBasicControls::UpdateStudioModeState(bool enabled)
{
	setClasses(ui->modeSwitch, enabled ? "state-active" : "");
}

void OBSBasicControls::EnableBroadcastFlow(bool enabled)
{
	ui->broadcastButton->setVisible(enabled);
	ui->broadcastButton->setEnabled(enabled);

	ui->broadcastButton->setText(tr("setup broadcast flow 🤑🤑🤑"));

	ui->broadcastButton->setProperty("broadcastState", "idle");
	ui->broadcastButton->style()->unpolish(ui->broadcastButton);
	ui->broadcastButton->style()->polish(ui->broadcastButton);
}

void OBSBasicControls::EnableReplayBufferButtons(bool enabled)
{
	ui->replayBufferButton->setVisible(enabled);
}

void OBSBasicControls::EnableVirtualCamButtons()
{
	ui->virtualCamButton->setVisible(true);
	ui->virtualCamConfigButton->setVisible(true);
}
