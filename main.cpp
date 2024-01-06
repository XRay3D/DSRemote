

#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#include <set>

#include "mainwindow.h"

auto messageHandler = qInstallMessageHandler(nullptr);
void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    auto file = context.file;
    // if(type == QtInfoMsg) return;
    QMessageLogContext& context_ = const_cast<QMessageLogContext&>(context);
    while(file && *file)
        if(std::set{'/', '\\'}.contains(*file++))
            context_.file = file;

    // QString data{context_.function};
    // data.replace(QRegularExpression(R"((\w+\:\:))"), "");
    // context_.function = data.toUtf8().data();
    messageHandler(type, context, message);
}

int main(int argc, char* argv[]) {
    qInstallMessageHandler(myMessageHandler);
    qSetMessagePattern(QLatin1String(
        "%{if-critical}\x1b[38;2;255;0;0m"
        "C %{endif}"
        "%{if-debug}\x1b[38;2;196;196;196m"
        "D %{endif}"
        "%{if-fatal}\x1b[1;38;2;255;0;0m"
        "F %{endif}"
        "%{if-info}\x1b[38;2;128;255;255m"
        "I %{endif}"
        "%{if-warning}\x1b[38;2;255;128;0m"
        "W %{endif}"
        // "%{time HH:mm:ss.zzz} "
        // "%{appname} %{pid} %{threadid} "
        // "%{type} "
        // "%{file}:%{line} %{function} "
        "%{if-category}%{category}%{endif}%{message} "
        "\x1b[38;2;64;64;64m <- %{function} <- %{file} : %{line}\x1b[0m"));

    QApplication app(argc, argv);

    //  app.setAttribute(Qt::AA_DontUseNativeMenuBar);

#if QT_VERSION >= 0x050000
    qApp->setStyle(QStyleFactory::create("Fusion"));
#endif
    qApp->setStyleSheet("QLabel, QMessageBox { messagebox-text-interaction-flags: 5; }");

    class UiMainWindow MainWindow;

    return app.exec();
}
