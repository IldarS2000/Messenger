#include <QDebug>
#include <QDateTime>
#include <string>

static std::string parseFileName(const std::string& filePath)
{
#ifdef _WIN32
    std::string::size_type filePos = filePath.rfind('\\');
#else
    std::string::size_type filePos = filePath.rfind('/');
#endif
    if (filePos != std::string::npos) {
        ++filePos;
    } else {
        filePos = 0;
    }
    return filePath.substr(filePos);
}

void messageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QByteArray localMsg = msg.toLocal8Bit();

    std::string filePath = context.file ? context.file : "";
    std::string fileName = parseFileName(filePath);

    const char* function = context.function ? context.function : "";
    std::string level;

    switch (type) {
        case QtDebugMsg:
            level = "DEBUG";
            break;
        case QtInfoMsg:
            level = "INFO";
            break;
        case QtWarningMsg:
            level = "WARNING";
            break;
        case QtCriticalMsg:
            level = "CRITICAL";
            break;
        case QtFatalMsg:
            level = "FATAL";
            break;
    }
    fprintf(stdout, "[%s] [%s] [%s] [%s:%u] %s\n", dateTime.toStdString().c_str(), level.c_str(), fileName.c_str(),
            function, context.line, localMsg.constData());
}