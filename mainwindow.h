#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//Qt
#include <QMainWindow>
#include <QStringList>

//Local
#include "ccMainAppInterface.h"
#include "FileIOFilter.h"

class ccGLWindow;
class ccHObject;
class ccStdPluginInterface;
class ccPickingHub;
class ccPointPropertiesDlg;
class CGAboutDialog;
class CGConsoleView;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initUi();
    void initConnections();
    void initCC();
    void initPointPicking();

public:
    void addToDB(ccHObject* entity);
	
	// inherit ccMainAppInterface (for plugins)
	
public slots:
    void addToDB(const QStringList filenames);
    void addToDBFilter(const QStringList &filenames, QString fileFilter = QString(), ccGLWindow* destWin = nullptr);

protected:
    void loadPlugins();
    void updateGLFrameGradient();
    bool checkForLoadedEntities();

protected:
    ccGLWindow* m_glWindow;
    ccHObject* m_selectedObject;
	ccHObject::Container m_selectedEntities;
    ccPickingHub* m_pickingHub;
    ccPointPropertiesDlg* m_ppDlg;

    QList<ccStdPluginInterface *> m_stdPlugins;

private:
    CGAboutDialog* m_AboutDlg;

private slots:
    void updateDisplay();
    void selectEntity(ccHObject* entity);
    void handleSelectionChanged();

    void on_action_new_triggered();
    void on_action_open_triggered();
    void on_action_openImage_triggered();
    void on_action_openPointCloud_triggered();
    void on_action_savePointCloud_triggered();
    void on_action_exit_triggered();
    void on_action_about_triggered();

    void on_action_SetViewTop_triggered();
    void on_action_SetViewFront_triggered();
    void on_action_SetViewLeft_triggered();
    void on_action_SetViewBack_triggered();
    void on_action_SetViewRight_triggered();
    void on_action_SetViewBottom_triggered();
    void on_action_SetViewIso1_triggered();
    void on_action_SetViewIso2_triggered();
    void on_action_GlobalZoom_triggered();
    void on_action_ClearAll_triggered();
    void on_action_FullScreen_triggered();
    void on_action_SetPivotOn_triggered();
    void on_action_SetPivotOff_triggered();
    void on_action_SetPivotRotationOnly_triggered();
    void on_action_Console_triggered(bool checked);
    void on_action_PointPicking_triggered();

    void on_action_Elevation_triggered();
    void on_action_Depth_triggered();
    void on_action_Intensity_triggered();

    void doEnableGLFilter();
    void doDisableGLFilter();

protected:
    void resizeEvent(QResizeEvent *event);
    void changeEvent(QEvent *event);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
