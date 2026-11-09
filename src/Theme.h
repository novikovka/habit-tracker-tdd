#pragma once
#include <QString>

inline QString applicationStyle()
{
    return QStringLiteral(R"CSS(
QMainWindow, QDialog { background: #f8f1e9; color: #59464a; }
QWidget { color: #59464a; }
QLabel { background: transparent; }
QLabel#appTitle { font-size: 29px; font-weight: 700; color: #754b5b; }
QLabel#subtitle { color: #927a78; font-size: 13px; }
QLabel#quote { background: #f4dce5; border: 1px solid #eac3d1; border-radius: 15px; padding: 18px; color: #87566a; font-size: 15px; }
QLabel#weekRange { font-size: 17px; font-weight: 600; color: #7b5362; }
QLabel#errorLabel, QLabel#nameError { background: #fbe5e4; color: #994e51; border-radius: 8px; padding: 10px; }
QLabel#emptyHint { background: #fffaf5; border-radius: 12px; padding: 22px; color: #927a78; }
QLabel#statsTitle { font-size: 25px; font-weight: 700; color: #754b5b; }
QLabel#total7, QLabel#total30 { background: #efd4df; color: #9b4e6c; font-size: 25px; font-weight: 700; padding: 10px 24px; border-radius: 12px; }
QPushButton { background: #efd5df; border: 1px solid #e2baca; border-radius: 9px; padding: 10px 16px; color: #75465a; font-weight: 600; }
QPushButton:hover { background: #e9c3d2; border-color: #c889a3; }
QPushButton:pressed { background: #dfadc0; }
QPushButton:focus { border: 2px solid #b86687; }
QPushButton:disabled { color: #b2a39f; background: #eee8e2; border-color: #e4dcd5; }
QPushButton#addHabit { background: #b65e7c; color: #ffffff; border-color: #b65e7c; }
QPushButton#addHabit:hover { background: #a64d6c; }
QPushButton#addHabit:disabled { background: #eee8e2; color: #b2a39f; border-color: #e4dcd5; }
QPushButton[compact="true"] { font-size: 12px; padding: 7px 9px; }
QPushButton[danger="true"] { background: #fff8f5; color: #a45e6d; border-color: #e8cbd0; }
QLineEdit { background: #fffdfb; border: 1px solid #dbb9c6; border-radius: 9px; padding: 10px; selection-background-color: #d897af; }
QLineEdit:focus { border: 2px solid #ba6b8a; }
QTableWidget { background: #fffdf9; alternate-background-color: #fbf5ef; border: 1px solid #eadbd4; border-radius: 12px; gridline-color: #f0e6df; selection-background-color: #f1dfd9; selection-color: #59464a; }
QTableWidget::item { padding: 8px; }
QTableWidget::item:focus { border: 1px solid #bb7690; }
QHeaderView::section { background: #f1e1df; border: none; border-right: 1px solid #f8eee9; padding: 9px 4px; color: #795563; font-weight: 600; }
QTableWidget::indicator { width: 21px; height: 21px; }
QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: #f8f1e9; }
QScrollBar:vertical { background: #f1e7df; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: #d8bac3; border-radius: 5px; min-height: 32px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)CSS");
}
