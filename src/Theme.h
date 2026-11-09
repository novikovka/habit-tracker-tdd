#pragma once
#include <QString>

inline QString applicationStyle()
{
    return QStringLiteral(R"CSS(
QMainWindow, QDialog { background: #f8f1e9; color: #59464a; }
QWidget { color: #403a3c; font-family: "Segoe UI"; font-size: 13px; }
QLabel { background: transparent; }
QLabel#appTitle { font-size: 29px; font-weight: 700; color: #754b5b; }
QLabel#subtitle { color: #927a78; font-size: 13px; }
QLabel#quote { background: #fffaf6; border: 1px solid #eadfe0; border-left: 3px solid #d4a1b5; border-radius: 14px; padding: 16px; color: #705560; font-size: 15px; }
QLabel#weekRange { font-size: 17px; font-weight: 600; color: #7b5362; }
QLabel#errorLabel, QLabel#nameError { background: #fbe5e4; color: #994e51; border-radius: 8px; padding: 10px; }
QLabel#emptyHint { background: #fffaf5; border-radius: 12px; padding: 22px; color: #927a78; }
QLabel#statsTitle { font-size: 25px; font-weight: 700; color: #754b5b; }
QLabel#total7, QLabel#total30 { background: #fffdfb; border: 1px solid #e8ddd8; color: #914866; font-size: 24px; font-weight: 600; padding: 6px 18px; border-radius: 10px; }
QPushButton { background: #fffdfb; border: 1px solid #e5d8dc; border-radius: 9px; padding: 10px 16px; color: #72475a; font-weight: 600; }
QPushButton:hover { background: #faedf2; border-color: #c994aa; }
QPushButton:pressed { background: #f0d7e2; }
QPushButton:focus { border: 2px solid #b86687; }
QPushButton:disabled { color: #b2a39f; background: #eee8e2; border-color: #e4dcd5; }
QPushButton#addHabit { background: #b65e7c; color: #ffffff; border-color: #b65e7c; }
QPushButton#addHabit:hover { background: #a64d6c; }
QPushButton#addHabit:disabled { background: #eee8e2; color: #b2a39f; border-color: #e4dcd5; }
QPushButton[compact="true"] { font-size: 12px; padding: 7px 9px; }
QPushButton[danger="true"] { background: #fff8f5; color: #a45e6d; border-color: #e8cbd0; }
QLineEdit { background: #fffdfb; border: 1px solid #dbb9c6; border-radius: 9px; padding: 10px; selection-background-color: #d897af; }
QLineEdit:focus { border: 2px solid #ba6b8a; }
QTableWidget { background: #fffdfb; alternate-background-color: #fcfaf7; border: 1px solid #e8ded8; border-radius: 12px; gridline-color: #f0e9e3; selection-background-color: #f8edf1; selection-color: #403a3c; }
QTableWidget::item { padding: 8px; }
QTableWidget::item:focus { border: 1px solid #bb7690; }
QHeaderView::section { background: #fffaf6; border: none; border-bottom: 1px solid #eadfdc; padding: 9px 4px; color: #64535a; font-weight: 600; }
QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: #f8f1e9; }
QScrollBar:vertical { background: #f1e7df; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: #d8bac3; border-radius: 5px; min-height: 32px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)CSS");
}
