#pragma once
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QString>

class QLineEdit;
class QTextBrowser;
class QPushButton;
class QLabel;

struct ChatMessage {
	QString role;
	QString content;
};

class OBSGroqChat : public QDialog {
	Q_OBJECT

public:
	OBSGroqChat(QWidget *parent = nullptr);
	~OBSGroqChat();

private:
	void SendChatMessage();
	void OnReply(QNetworkReply *reply);
	void LoadApiKey();
	void SaveApiKey();
	void SetApiKeyState();
	void SetCatEmotion(const QString &emotion);
	void DetectEmotion(const QString &text);
	void OnEmotionReply(QNetworkReply *reply);

	QLineEdit            *apiKeyInput;
	QTextBrowser         *chatHistory;
	QLineEdit            *messageInput;
	QPushButton          *sendButton;
	QLabel               *catImageLabel;
	QNetworkAccessManager *net;
	QNetworkAccessManager *emotionNet;
	QNetworkReply        *pendingEmotionReply = nullptr; // track in-flight emotion request

	QList<ChatMessage> history;
	QString currentEmotion = "curious"; // initialised so SetCatEmotion works at construction
	QString systemPrompt;

	static constexpr const char *groqUrl    = "https://api.groq.com/openai/v1/chat/completions";
	static constexpr int          maxHistory = 50;
};
