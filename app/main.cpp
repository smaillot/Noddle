#include <QApplication>
#include "NoddleMainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Noddle");

    // Dark-theme QSS scoped to QGraphicsView descendants so it only
    // affects embedded node widgets, not the main application chrome.
    // #nodePreview labels get an explicit dark background override.
    app.setStyleSheet(R"(
        QGraphicsView QComboBox,
        QGraphicsView QPushButton,
        QGraphicsView QSpinBox,
        QGraphicsView QDoubleSpinBox,
        QGraphicsView QLabel {
            background: transparent;
            color: #ddd;
            border: none;
        }
        QGraphicsView QLabel#nodePreview {
            background: #222;
            border: 1px solid #555;
        }
        QGraphicsView QComboBox {
            border: 1px solid #555;
            border-radius: 3px;
            padding: 2px 6px;
        }
        QGraphicsView QComboBox::drop-down {
            border: none;
        }
        QGraphicsView QComboBox QAbstractItemView {
            background: #2d2d2d;
            color: #ddd;
            selection-background-color: #505050;
        }
        QGraphicsView QPushButton {
            background: rgba(58, 58, 58, 180);
            border: 1px solid #555;
            border-radius: 3px;
            padding: 3px 10px;
        }
        QGraphicsView QPushButton:hover {
            background: rgba(72, 72, 72, 200);
        }
        QGraphicsView QPushButton:pressed {
            background: rgba(42, 42, 42, 200);
        }
        QGraphicsView QSpinBox,
        QGraphicsView QDoubleSpinBox {
            border: 1px solid #555;
            border-radius: 3px;
            padding: 2px 4px;
        }
        QGraphicsView QWidget {
            background: transparent;
        }
    )");

    NoddleMainWindow window;
    window.show();

    return app.exec();
}
