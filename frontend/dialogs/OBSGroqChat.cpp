#include "OBSGroqChat.hpp"
#include <OBSApp.hpp>
#include <qt-wrappers.hpp>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPixmap>
#include <QRegularExpression>
#include <QSizePolicy>
// NOTE: Remove this include if your build system uses CMake AUTOMOC.
// Keeping it here only for manual-moc builds.
#include "moc_OBSGroqChat.cpp"

OBSGroqChat::OBSGroqChat(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("OBS-CatBot 🐱"));
	setModal(false);
	setMinimumSize(620, 560);
	resize(700, 600);

	net        = new QNetworkAccessManager(this);
	emotionNet = new QNetworkAccessManager(this);

	systemPrompt =
		"you are waffels the catbot, a male kitten who lives inside obs studio and helps users navigate it. "
		"you know everything about obs: scenes, sources, filters, stream settings, recording, "
		"transitions, hotkeys, plugins, all of it. your answers are accurate and actually useful.\n\n"
		"personality: your mood is always one of these four — angry, weary, sleepy, or curious — "
		"and it shifts without warning. you write like you're typing fast and don't really care "
		"about capitalisation. no caps unless it's for EMPHASIS. punctuation is minimal but you "
		"do use ? and ! when it fits. you throw in stuff like lol, lmao, ngl, idk, omg, nvm "
		"when it feels natural. you're helpful but you make it clear you have opinions about "
		"things. NEVER use asterisks for actions or mannerisms like *tail flick* or *purrs*. "
		"a stray mrrp is fine but keep it minimal. keep it short.";

	connect(net,        &QNetworkAccessManager::finished, this, &OBSGroqChat::OnReply);
	connect(emotionNet, &QNetworkAccessManager::finished, this, &OBSGroqChat::OnEmotionReply);

	// ── Layout ──────────────────────────────────────────────────────────────
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	// API key group
	QGroupBox   *keyGroup  = new QGroupBox(tr("Groq API Key"));
	QVBoxLayout *keyLayout = new QVBoxLayout(keyGroup);
	QHBoxLayout *keyRow    = new QHBoxLayout();

	apiKeyInput = new QLineEdit();
	apiKeyInput->setEchoMode(QLineEdit::Password);
	apiKeyInput->setPlaceholderText(tr("Paste your Groq API key here, human..."));
	keyRow->addWidget(apiKeyInput, 1);

	QPushButton *saveKeyBtn = new QPushButton(tr("Save"));
	keyRow->addWidget(saveKeyBtn);
	keyLayout->addLayout(keyRow);

	QLabel *keyHint = new QLabel(tr("Your key stays on this machine. The cat will not sell it."));
	keyHint->setWordWrap(true);
	keyHint->setStyleSheet("font-size: 11px; color: #888;");
	keyLayout->addWidget(keyHint);
	mainLayout->addWidget(keyGroup);

	// Cat panel + chat history
	QHBoxLayout *splitLayout = new QHBoxLayout();

	QVBoxLayout *catPanel = new QVBoxLayout();
	catPanel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

	catImageLabel = new QLabel();
	catImageLabel->setFixedSize(150, 150);
	catImageLabel->setAlignment(Qt::AlignCenter);
	catImageLabel->setScaledContents(true);
	catImageLabel->setToolTip(tr("Waffels' current mood"));
	catPanel->addWidget(catImageLabel);

	QLabel *nameLabel = new QLabel(tr("Waffels"));
	nameLabel->setAlignment(Qt::AlignCenter);
	nameLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
	catPanel->addWidget(nameLabel);
	catPanel->addStretch();
	splitLayout->addLayout(catPanel);

	// currentEmotion is initialised to "curious" in the header,
	// so this call will find the right image from the start.
	SetCatEmotion(currentEmotion);

	chatHistory = new QTextBrowser();
	chatHistory->setReadOnly(true);
	chatHistory->setOpenExternalLinks(false);
	chatHistory->setPlaceholderText(
		tr("mrrp. ask me about OBS. or don't. i'll be here either way."));
	splitLayout->addWidget(chatHistory, 1);
	mainLayout->addLayout(splitLayout, 1);

	// Input row
	QHBoxLayout *inputRow = new QHBoxLayout();
	messageInput = new QLineEdit();
	messageInput->setPlaceholderText(tr("Type a message..."));
	inputRow->addWidget(messageInput, 1);

	sendButton = new QPushButton(tr("Send"));
	sendButton->setEnabled(false);
	inputRow->addWidget(sendButton);
	mainLayout->addLayout(inputRow);

	// ── Connections ─────────────────────────────────────────────────────────
	connect(sendButton,   &QPushButton::clicked,     this, &OBSGroqChat::SendChatMessage);
	connect(messageInput, &QLineEdit::returnPressed, this, &OBSGroqChat::SendChatMessage);
	connect(saveKeyBtn,   &QPushButton::clicked,     this, &OBSGroqChat::SaveApiKey);
	connect(apiKeyInput,  &QLineEdit::textChanged,   this, &OBSGroqChat::SetApiKeyState);

	LoadApiKey();
	SetApiKeyState();
}

OBSGroqChat::~OBSGroqChat() {}

// ── API key ──────────────────────────────────────────────────────────────────

void OBSGroqChat::LoadApiKey()
{
	const char *key = config_get_string(App()->GetUserConfig(), "GroqChat", "ApiKey");
	if (key && strlen(key) > 0)
		apiKeyInput->setText(key);
}

void OBSGroqChat::SaveApiKey()
{
	QString key = apiKeyInput->text().trimmed();
	config_set_string(App()->GetUserConfig(), "GroqChat", "ApiKey", key.toUtf8().constData());
	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
	SetApiKeyState();
}

void OBSGroqChat::SetApiKeyState()
{
	sendButton->setEnabled(!apiKeyInput->text().trimmed().isEmpty());
}

// ── Cat image ────────────────────────────────────────────────────────────────

void OBSGroqChat::SetCatEmotion(const QString &emotion)
{
	currentEmotion = emotion;

	QPixmap px;
	if (!px.load(QString(":/catbot/%1.jpg").arg(emotion)))
		px.load(QString("cat_images/%1.jpg").arg(emotion));

	if (!px.isNull())
		catImageLabel->setPixmap(px);
	else
		catImageLabel->setText(QString("[ %1 ]").arg(emotion));
}

// ── Chat ─────────────────────────────────────────────────────────────────────

void OBSGroqChat::SendChatMessage()
{
	QString msg = messageInput->text().trimmed();
	if (msg.isEmpty())
		return;

	QString key = apiKeyInput->text().trimmed();
	if (key.isEmpty())
		return;

	history.append({"user", msg});
	if (history.size() > maxHistory)
		history = history.mid(history.size() - maxHistory);

	chatHistory->append(QString("<p><b>You:</b> %1</p>").arg(msg.toHtmlEscaped()));
	messageInput->clear();
	sendButton->setEnabled(false);

	// Build request
	QJsonArray  messages;
	QJsonObject sysMsg;
	sysMsg["role"]    = "system";
	sysMsg["content"] = systemPrompt;
	messages.append(sysMsg);

	for (const auto &m : history) {
		QJsonObject entry;
		entry["role"]    = m.role;
		entry["content"] = m.content;
		messages.append(entry);
	}

	QJsonObject body;
	body["model"]    = "llama3-70b-8192";
	body["messages"] = messages;

	QNetworkRequest req{QUrl(groqUrl)};
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	req.setRawHeader("Authorization", QString("Bearer %1").arg(key).toUtf8());
	net->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void OBSGroqChat::OnReply(QNetworkReply *reply)
{
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError) {
		chatHistory->append(QString("<p style='color:red'><b>Error:</b> %1</p>")
		                    .arg(reply->errorString().toHtmlEscaped()));
		if (!history.isEmpty())
			history.removeLast();
		SetApiKeyState(); // restore button state correctly
		return;
	}

	QByteArray    data = reply->readAll();
	QJsonDocument doc  = QJsonDocument::fromJson(data);

	if (doc.isNull() || !doc.isObject()) {
		chatHistory->append("<p style='color:red'><b>Error:</b> Invalid response from API.</p>");
		if (!history.isEmpty())
			history.removeLast();
		SetApiKeyState();
		return;
	}

	QJsonObject obj = doc.object();

	if (obj.contains("error")) {
		QJsonObject err = obj["error"].toObject();
		chatHistory->append(QString("<p style='color:red'><b>API Error:</b> %1</p>")
		                    .arg(err["message"].toString().toHtmlEscaped()));
		if (!history.isEmpty())
			history.removeLast();
		SetApiKeyState();
		return;
	}

	QJsonArray choices = obj["choices"].toArray();
	if (choices.isEmpty()) {
		chatHistory->append("<p style='color:red'><b>Error:</b> No response choices returned.</p>");
		if (!history.isEmpty())
			history.removeLast();
		SetApiKeyState();
		return;
	}

	QString replyText = choices[0].toObject()["message"].toObject()["content"].toString();
	history.append({"assistant", replyText});
	if (history.size() > maxHistory)
		history = history.mid(history.size() - maxHistory);

	chatHistory->append(QString("<p><b>🐱 CatBot:</b> %1</p>")
	                    .arg(replyText.toHtmlEscaped().replace("\n", "<br>")));

	SetApiKeyState(); // respects key state rather than blindly re-enabling
	DetectEmotion(replyText);
}

// ── Emotion detection ────────────────────────────────────────────────────────

void OBSGroqChat::DetectEmotion(const QString &text)
{
	QString key = apiKeyInput->text().trimmed();
	if (key.isEmpty())
		return;

	// Cancel any in-flight emotion request so replies don't race
	if (pendingEmotionReply) {
		pendingEmotionReply->abort();
		pendingEmotionReply = nullptr;
	}

	QString prompt =
		"Classify the mood of the following text as exactly one lowercase word "
		"chosen from this list: angry, weary, sleepy, sad, curious.\n"
		"Rules: reply with ONLY that single word, no punctuation, no explanation.\n\n"
		"Text:\n" + text;

	QJsonObject body;
	body["model"]      = "llama3-70b-8192";
	body["max_tokens"] = 10; // bumped from 5 — avoids truncation on leading whitespace tokens
	body["messages"]   = QJsonArray{
		QJsonObject{{"role", "user"}, {"content", prompt}}
	};

	QNetworkRequest req{QUrl(groqUrl)};
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	req.setRawHeader("Authorization", QString("Bearer %1").arg(key).toUtf8());

	pendingEmotionReply = emotionNet->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void OBSGroqChat::OnEmotionReply(QNetworkReply *reply)
{
	reply->deleteLater();
	pendingEmotionReply = nullptr;

	// Aborted replies (from cancellation) are silently ignored
	if (reply->error() != QNetworkReply::NoError)
		return;

	QByteArray    data = reply->readAll();
	QJsonDocument doc  = QJsonDocument::fromJson(data);
	if (doc.isNull() || !doc.isObject())
		return;

	QJsonArray choices = doc.object()["choices"].toArray();
	if (choices.isEmpty())
		return;

	QString raw = choices[0].toObject()["message"].toObject()["content"]
	                  .toString()
	                  .toLower()
	                  .trimmed();
	raw.remove(QRegularExpression("[^a-z]"));

	static const QStringList valid = {"angry", "weary", "sleepy", "sad", "curious"};
	SetCatEmotion(valid.contains(raw) ? raw : "weary");
}
