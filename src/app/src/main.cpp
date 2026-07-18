// AtivaStage — application entry point.
//
// Phase 1, Marco 1.0: boots the Qt Quick engine and shows a single empty window.
// This exists to prove the GUI toolchain (Qt Quick + RHI) and the packaging path
// (.app / .exe) compile and run on both platforms before any engine code lands.
//
// UI text is pt-BR (invariant); code/comments are English (invariant).

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QGuiApplication::setApplicationName("AtivaStage");
    QGuiApplication::setOrganizationName("Ativa");
    QGuiApplication::setOrganizationDomain("br.com.ativa");

    QQmlApplicationEngine engine;

    // Fail fast if the root QML object cannot be created (e.g. missing module).
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("AtivaStage", "Main");

    return app.exec();
}
