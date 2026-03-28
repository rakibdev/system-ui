#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

extern "C" {

void* QApplication_new(int argc, char** argv) {
    static int s_argc = argc;
    static char** s_argv = argv;
    return new QApplication(s_argc, s_argv);
}

void QApplication_delete(void* app) {
    delete static_cast<QApplication*>(app);
}

int QApplication_exec(void* app) {
    return static_cast<QApplication*>(app)->exec();
}

void* QWidget_new() {
    return new QWidget();
}

void QWidget_delete(void* w) {
    delete static_cast<QWidget*>(w);
}

void QWidget_setWindowTitle(void* w, const char* title) {
    static_cast<QWidget*>(w)->setWindowTitle(QString::fromUtf8(title));
}

void QWidget_resize(void* w, int width, int height) {
    static_cast<QWidget*>(w)->resize(width, height);
}

void QWidget_show(void* w) {
    static_cast<QWidget*>(w)->show();
}

void QWidget_setLayout(void* w, void* layout) {
    static_cast<QWidget*>(w)->setLayout(static_cast<QLayout*>(layout));
}

void* QVBoxLayout_new(void* parent) {
    return new QVBoxLayout(static_cast<QWidget*>(parent));
}

void QBoxLayout_addWidget(void* layout, void* widget, int stretch, int alignment) {
    static_cast<QBoxLayout*>(layout)->addWidget(static_cast<QWidget*>(widget), stretch, static_cast<Qt::Alignment>(alignment));
}

void* QLabel_new(const char* text) {
    return new QLabel(QString::fromUtf8(text));
}

void QLabel_setText(void* label, const char* text) {
    static_cast<QLabel*>(label)->setText(QString::fromUtf8(text));
}

void QLabel_setAlignment(void* label, int alignment) {
    static_cast<QLabel*>(label)->setAlignment(static_cast<Qt::Alignment>(alignment));
}

void* QPushButton_new(const char* text, void* parent) {
    return new QPushButton(QString::fromUtf8(text), static_cast<QWidget*>(parent));
}

typedef void (*ClickCallback)();

class ButtonHandler : public QObject {
    ClickCallback callback;
public:
    ButtonHandler(ClickCallback cb, QObject* parent = nullptr) : QObject(parent), callback(cb) {}
public slots:
    void onClick() { if (callback) callback(); }
};

void QPushButton_onClicked(void* button, ClickCallback callback) {
    auto* handler = new ButtonHandler(callback, static_cast<QPushButton*>(button));
    QObject::connect(static_cast<QPushButton*>(button), &QPushButton::clicked, handler, &ButtonHandler::onClick);
}

}
