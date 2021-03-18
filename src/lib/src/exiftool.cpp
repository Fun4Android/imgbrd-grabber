#include "exiftool.h"
#include <QProcess>
#include"logger.h"


Exiftool::Exiftool(QObject *parent)
	: QObject(parent)
{
	connect(&m_process, &QProcess::readyReadStandardError, this, &Exiftool::onError);
}


bool Exiftool::start(int msecs)
{
	if (m_process.state() != QProcess::NotRunning) {
		return true;
	}

	const QString program = "exiftool";
	const QStringList arguments { "-stay_open", "True", "-@", "-" };

	m_process.start(program, arguments);
	return m_process.waitForBytesWritten(msecs);
}

bool Exiftool::setMetadata(const QString &file, const QMap<QString, QString> &metadata, int msecs)
{
	QStringList commands;
	commands.append({ "-sep", ";" });
	for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
		commands.append("-" + it.key() + "=" + it.value());
	}
	commands.append("-overwrite_original");

	return execute(file, commands.join("\n"), msecs);
}

bool Exiftool::execute(const QString &file, const QString &command, int msecs)
{
	QString toWrite = (command.isEmpty() ? "" : command + "\n") + file + "\n-execute\n";
	m_process.write(toWrite.toLocal8Bit());

	m_process.setReadChannel(QProcess::StandardOutput);

	while (m_process.waitForReadyRead(msecs)) {
		QString output = QString::fromLocal8Bit(m_process.readAllStandardOutput()).trimmed();
		log(QString("[Exiftool] %1").arg(output), Logger::Debug);
		if (output.endsWith("{ready}")) {
			return true;
		}
	}

	return false;
}

bool Exiftool::stop(int msecs)
{
	m_process.write("-stay_open\nFalse\n");
	m_process.closeWriteChannel();
	return m_process.waitForFinished(msecs);
}


void Exiftool::onError()
{
	const QString output = QString::fromLocal8Bit(m_process.readAllStandardError()).trimmed();
	const Logger::LogLevel logLevel = output.startsWith("Warning:") ? Logger::Warning : Logger::Error;
	log(QString("[Exiftool] %1").arg(output), logLevel);
}