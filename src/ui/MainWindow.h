#pragma once
#include <QMainWindow>
#include "model/FolderTreeModel.h"
#include "model/EntryTableModel.h"

class AppController;
class PreviewPane;
class QTreeView;
class QTableView;
class QSplitter;
class QLabel;
class QLineEdit;
class QToolBar;
class QAction;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppController* controller, QWidget* parent = nullptr);
    ~MainWindow();

    void updateTitle();
    void updateStatusBar();

public slots:
    void onArchiveOpened();
    void onArchiveClosed();
    void onTreeUpdated();
    void onVerifyComplete(const VerifyReport& report);

private slots:
    void openArchive();
    void createArchive();
    void closeArchive();
    void extractArchive();
    void addFile();
    void addFolder();
    void deleteSelected();
    void goUp();
    void refresh();
    void verifyArchive();
    void showProperties();
    void showStatistics();

    void onTreeClicked(const QModelIndex& index);
    void onEntryActivated(const QModelIndex& index);
    void onEntryClicked(const QModelIndex& index);
    void onCustomContextMenu(const QPoint& point);
    void onAddressReturnPressed();
    void navigateToPath(const QString& path);

private:
    void setupMenus();
    void setupToolbar();
    void setupCentralWidget();
    void setupStatusBar();
    void connectSignals();

    void updateEntryView(const QString& dirPath);
    void collectSelectedPaths(QStringList& paths);
    ArchiveTreeNode* findNode(const QString& path);

    AppController* controller_;

    // UI elements
    QToolBar* toolbar_ = nullptr;
    QLineEdit* addressEdit_ = nullptr;
    QSplitter* mainSplitter_ = nullptr;
    QTreeView* folderTree_ = nullptr;
    QTableView* entryTable_ = nullptr;
    PreviewPane* previewPane_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    // Models
    FolderTreeModel* folderModel_ = nullptr;
    EntryTableModel* entryModel_ = nullptr;

    // Current navigation state
    QString currentPath_;

    // Actions
    QAction* actOpen_ = nullptr;
    QAction* actCreate_ = nullptr;
    QAction* actExtract_ = nullptr;
    QAction* actAddFile_ = nullptr;
    QAction* actAddFolder_ = nullptr;
    QAction* actDelete_ = nullptr;
    QAction* actUp_ = nullptr;
    QAction* actRefresh_ = nullptr;
    QAction* actVerify_ = nullptr;
    QAction* actProperties_ = nullptr;
    QAction* actStats_ = nullptr;
    QAction* actClose_ = nullptr;
};
