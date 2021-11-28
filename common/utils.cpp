#include <QDebug>
#include <QDateTime>
#include <QFileInfo>

void messageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    QTextStream out(stdout);
    out << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "]" << ' ';

    const QFileInfo file(context.file);
    switch (type) {
        case QtDebugMsg: {
            out << "[DEBUG]" << ' ';
            out << "[" << file.fileName() << "]" << ' ';
            out << "[" << context.function << ":" << context.line << "]";
            break;
        }
        case QtInfoMsg:
            out << "[INFO]";
            break;
        case QtWarningMsg:
            out << "[WARNING]";
            break;
        case QtCriticalMsg:
            out << "[CRITICAL]";
            break;
        case QtFatalMsg:
            out << "[FATAL]";
            break;
    }
    out << ' ' << message.trimmed() << '\n';
    out.flush();
}