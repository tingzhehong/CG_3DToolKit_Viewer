#include "mainwindow.h"
#include "ui_mainwindow.h"

//Qt
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QDebug>

//CC
#include <CCCommon.h>
#include <ccPickingHub.h>
#include <ccPointPropertiesDlg.h>
#include <ccPersistentSettings.h>

//CG
#include <CGAboutDialog.h>
#include <CGConsoleView.h>

static const QString s_fileFilterSeparator(";;");
static QFileDialog::Options CCFileDialogOptions()
{
    //dialog options
    QFileDialog::Options dialogOptions = QFileDialog::Options();
    if (!ccOptions::Instance().useNativeDialogs)
    {
        dialogOptions |= QFileDialog::DontUseNativeDialog;
    }
    return dialogOptions;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_glWindow(nullptr)
    , m_selectedObject(nullptr)
    , m_pickingHub(nullptr)
    , m_ppDlg(nullptr)
    , m_AboutDlg(new CGAboutDialog())
{
    ui->setupUi(this);

    initUi();
    initConnections();
    initCC();
    loadPlugins();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_AboutDlg;
    delete m_pickingHub;
    delete m_ppDlg;
    
}

void MainWindow::initUi()
{
    //insert GL window in a vertical layout
    {
        QVBoxLayout* verticalLayout = new QVBoxLayout(ui->GLframe);
        verticalLayout->setSpacing(0);
        const int margin = 10;
        verticalLayout->setContentsMargins(margin, margin, margin, margin);

        QWidget* glWidget = nullptr;
        CreateGLWindow(m_glWindow, glWidget);
        assert(m_glWindow && glWidget);
        verticalLayout->addWidget(glWidget);

        ui->centralwidget->setLayout(verticalLayout);
        ui->gridLayout->addWidget(CGConsoleView::getInstance());
    }
    updateGLFrameGradient();

    m_glWindow->setRectangularPickingAllowed(false);
    m_glWindow->setShaderPath("shaders");
}

void MainWindow::initConnections()
{
    connect(m_glWindow,	&ccGLWindow::filesDropped, this, static_cast<void (MainWindow::*)(QStringList)>(&MainWindow::addToDB), Qt::QueuedConnection);
    connect(m_glWindow,	&ccGLWindow::entitySelectionChanged, this, &MainWindow::selectEntity);

}

void MainWindow::initCC()
{
    FileIOFilter::InitInternalFilters();
    FileIOFilter::ImportFilterList();
}

void MainWindow::addToDB(ccHObject *entity)
{
    assert(entity && m_glWindow);

    entity->setDisplay_recursive(m_glWindow);

    ccHObject* currentRoot = m_glWindow->getSceneDB();
    if (currentRoot)
    {
        //already a pure 'root'
        if (currentRoot->isA(CC_TYPES::HIERARCHY_OBJECT))
        {
            currentRoot->addChild(entity);
        }
        else
        {
            ccHObject* root = new ccHObject("root");
            root->addChild(currentRoot);
            root->addChild(entity);
            m_glWindow->setSceneDB(root);
			m_selectedObject = root;
            m_selectedEntities.push_back(root);
        }
    }
    else
    {
        m_glWindow->setSceneDB(entity);
        m_selectedObject = entity;
        m_selectedEntities.push_back(entity);
    }

    checkForLoadedEntities();
}

void MainWindow::initPointPicking()
{
    if (!m_pickingHub)
    {
        m_pickingHub = new ccPickingHub(nullptr, this);
        m_pickingHub->m_activeGLWindow = m_glWindow;
        m_pickingHub->addListener(m_ppDlg);
        m_pickingHub->togglePickingMode(true);

        connect(m_glWindow, &ccGLWindow::itemPicked, m_pickingHub, &ccPickingHub::processPickedItem, Qt::UniqueConnection);
        connect(m_glWindow, &QObject::destroyed, m_pickingHub, &ccPickingHub::onActiveWindowDeleted);
    }
}

bool MainWindow::checkForLoadedEntities()
{
    bool loadedEntities = true;
    m_glWindow->displayNewMessage(QString(), ccGLWindow::SCREEN_CENTER_MESSAGE); //clear (any) message in the middle area

    if (!m_glWindow->getSceneDB())
    {
        m_glWindow->displayNewMessage("Drag & drop files on the 3D window to load them!", ccGLWindow::SCREEN_CENTER_MESSAGE, true, 3600);
        loadedEntities = false;
    }

    if (m_glWindow->getDisplayParameters().displayCross != loadedEntities)
    {
        ccGui::ParamStruct params = m_glWindow->getDisplayParameters();
        params.displayCross = loadedEntities;
        m_glWindow->setDisplayParameters(params);
    }

    return loadedEntities;
}

void MainWindow::addToDB(const QStringList filenames)
{
    ccHObject* currentRoot = m_glWindow->getSceneDB();
    if (currentRoot)
    {
        m_selectedObject = nullptr;
        m_selectedEntities.clear();
        m_glWindow->setSceneDB(nullptr);
        m_glWindow->redraw();
        delete currentRoot;
        currentRoot = nullptr;
    }

    bool scaleAlreadyDisplayed = false;

    FileIOFilter::LoadParameters parameters;
    parameters.alwaysDisplayLoadDialog = false;
    parameters.shiftHandlingMode = ccGlobalShiftManager::NO_DIALOG_AUTO_SHIFT;
    parameters.parentWidget = this;

    const ccOptions& options = ccOptions::Instance();
    FileIOFilter::ResetSesionCounter();

    for (int i = 0; i < filenames.size(); ++i)
    {
        CC_FILE_ERROR result = CC_FERR_NO_ERROR;
        ccHObject* newGroup = FileIOFilter::LoadFromFile(filenames[i], parameters, result);

        if (newGroup)
        {
            if (!options.normalsDisplayedByDefault)
            {
                //disable the normals on all loaded clouds!
                ccHObject::Container clouds;
                newGroup->filterChildren(clouds, true, CC_TYPES::POINT_CLOUD);
                for (ccHObject* cloud : clouds)
                {
                    if (cloud)
                    {
                        static_cast<ccGenericPointCloud*>(cloud)->showNormals(false);
                    }
                }
            }

            addToDB(newGroup);

            if (!scaleAlreadyDisplayed)
            {
                for (unsigned i = 0; i < newGroup->getChildrenNumber(); ++i)
                {
                    ccHObject* ent = newGroup->getChild(i);
                    if (ent->isA(CC_TYPES::POINT_CLOUD))
                    {
                        ccPointCloud* pc = static_cast<ccPointCloud*>(ent);
                        if (pc->hasScalarFields())
                        {
                            pc->setCurrentDisplayedScalarField(0);
                            pc->showSFColorsScale(true);
                            scaleAlreadyDisplayed = true;
                        }
                    }
                    else if (ent->isKindOf(CC_TYPES::MESH))
                    {
                        ccGenericMesh* mesh = static_cast<ccGenericMesh*>(ent);
                        if (mesh->hasScalarFields())
                        {
                            mesh->showSF(true);
                            scaleAlreadyDisplayed=true;
                            ccPointCloud* pc = static_cast<ccPointCloud*>(mesh->getAssociatedCloud());
                            pc->showSFColorsScale(true);
                        }
                    }
                }
            }
        }

        if (result == CC_FERR_CANCELED_BY_USER)
        {
            //stop importing the file if the user has cancelled the current process!
            break;
        }
    }

    checkForLoadedEntities();
}

void MainWindow::addToDBFilter(const QStringList &filenames, QString fileFilter, ccGLWindow *destWin)
{
    //to use the same 'global shift' for multiple files
    CCVector3d loadCoordinatesShift(0,0,0);
    bool loadCoordinatesTransEnabled = false;

    FileIOFilter::LoadParameters parameters;
    {
        parameters.alwaysDisplayLoadDialog = true;
        parameters.shiftHandlingMode = ccGlobalShiftManager::DIALOG_IF_NECESSARY;
        parameters.coordinatesShift = &loadCoordinatesShift;
        parameters.coordinatesShiftEnabled = &loadCoordinatesTransEnabled;
        parameters.parentWidget = this;
    }

    const ccOptions& options = ccOptions::Instance();
    FileIOFilter::ResetSesionCounter();

    for (const QString &filename : filenames)
    {
        CC_FILE_ERROR result = CC_FERR_NO_ERROR;
        ccHObject* newGroup = FileIOFilter::LoadFromFile(filename, parameters, result, fileFilter);

        if (newGroup)
        {
            if (!options.normalsDisplayedByDefault)
            {
                //disable the normals on all loaded clouds!
                ccHObject::Container clouds;
                newGroup->filterChildren(clouds, true, CC_TYPES::POINT_CLOUD);
                for (ccHObject* cloud : clouds)
                {
                    if (cloud)
                    {
                        static_cast<ccGenericPointCloud*>(cloud)->showNormals(false);
                    }
                }
            }

            if (destWin)
            {
                newGroup->setDisplay_recursive(destWin);
            }

             addToDB(newGroup);
        }

        if (result == CC_FERR_CANCELED_BY_USER)
        {
            //stop importing the file if the user has cancelled the current process!
            break;
        }
    }

    QMainWindow::statusBar()->showMessage(QString("%1 file(s) loaded").arg(filenames.size()), 2000);
}

void MainWindow::loadPlugins()
{
    ui->menu_plugins->setEnabled(false);

    QStringList paths;
    paths << "plugins";
    ccPluginManager::get().setPaths(paths);
    ccPluginManager::get().loadPlugins();

    for (ccPluginInterface *plugin : ccPluginManager::get().pluginList())
    {
        if (plugin == nullptr)
        {
            Q_ASSERT(false);
            continue;
        }

        switch (plugin->getType())
        {
            //is this a standard plugin?
            //if ( plugin->getType() == CC_STD_PLUGIN )
            case CC_STD_PLUGIN:
            {
                ccStdPluginInterface *stdPlugin = static_cast<ccStdPluginInterface*>(plugin);

                const QString pluginName = stdPlugin->getName();

                Q_ASSERT(!pluginName.isEmpty());

                if (pluginName.isEmpty())
                {
                    // should be unreachable - we have already checked for this in ccPlugins::Find()
                    continue;
                }

                ccLog::Print( QStringLiteral("Plugin name: [%1] (std filter)").arg(pluginName));

                QAction* action = new QAction(pluginName, this);
                action->setToolTip(stdPlugin->getDescription());
                action->setIcon(stdPlugin->getIcon());

                QVariant v;
                v.setValue(stdPlugin);
                action->setData(v);

                //connect(action, &QAction::triggered, this, [&](){stdPlugin->start();});

                ui->menu_plugins->addAction(action);
                ui->menu_plugins->setEnabled(true);
                ui->menu_plugins->setVisible(true);

                m_stdPlugins.push_back(stdPlugin);

                break;
            }

            //is this a GL plugin?
            //if ( plugin->getType() == CC_GL_FILTER_PLUGIN )
            case CC_GL_FILTER_PLUGIN:
            {
                ccGLPluginInterface *glPlugin = static_cast<ccGLPluginInterface*>(plugin);

                const QString pluginName = glPlugin->getName();

                Q_ASSERT(!pluginName.isEmpty());

                if (pluginName.isEmpty())
                {
                    // should be unreachable - we have already checked for this in ccPlugins::Find()
                    continue;
                }

                ccLog::Print( QStringLiteral("Plugin name: [%1] (GL filter)").arg(pluginName));

                QAction* action = new QAction(pluginName, this);
                action->setToolTip(glPlugin->getDescription());
                action->setIcon(glPlugin->getIcon());

                // store the plugin's interface pointer in the QAction data so we can access it in doEnableGLFilter()
                QVariant v;
                v.setValue(glPlugin);
                action->setData(v);

                connect(action, &QAction::triggered, this, &MainWindow::doEnableGLFilter);

                ui->menu_plugins->addAction(action);
                ui->menu_plugins->setEnabled(true);
                ui->menu_plugins->setVisible(true);

                break;
            }
			
            //is this a I/O plugin?
			//if ( plugin->getType() == CC_IO_FILTER_PLUGIN )
			case CC_IO_FILTER_PLUGIN:
			{
                ccIOPluginInterface *ioPlugin = static_cast<ccIOPluginInterface*>(plugin);
                // there are no menus or toolbars for I/O plugins
				break;
			}
        }
    }

    //add this a remove GL filter
    QAction* action = new QAction("Remove", this);
    action->setToolTip("Remove GL Filter");
    action->setIcon(QIcon(":/CC/res/images/noFilter.png"));

    connect(action, &QAction::triggered, this, &MainWindow::doDisableGLFilter);

    ui->menu_plugins->addAction(action);
}

void MainWindow::updateGLFrameGradient()
{
    //display parameters
    static const ccColor::Rgbub s_black(0, 0, 0);
    static const ccColor::Rgbub s_white(255, 255, 255);
    bool stereoModeEnabled = m_glWindow->stereoModeIsEnabled();
    const ccColor::Rgbub& bkgCol = stereoModeEnabled ? s_black : m_glWindow->getDisplayParameters().backgroundCol;
    const ccColor::Rgbub& forCol = stereoModeEnabled ? s_white : m_glWindow->getDisplayParameters().pointsDefaultCol;

    QString styleSheet = QString("QFrame{border: 2px solid white; border-radius: 10px; background: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 rgb(%1,%2,%3), stop:1 rgb(%4,%5,%6));}")
                                .arg(bkgCol.r)
                                .arg(bkgCol.g)
                                .arg(bkgCol.b)
                                .arg(255-forCol.r)
                                .arg(255-forCol.g)
                                .arg(255-forCol.b);

    ui->GLframe->setStyleSheet(styleSheet);
}

void MainWindow::updateDisplay()
{
    updateGLFrameGradient();

    m_glWindow->redraw();
}

void MainWindow::selectEntity(ccHObject *toSelect)
{
    ccHObject* currentRoot = m_glWindow->getSceneDB();
    if (!currentRoot)
        return;

    currentRoot->setSelected_recursive(false);

    if (toSelect)
    {
        toSelect->setSelected(true);
        m_selectedObject = toSelect;

        handleSelectionChanged();
    }

    m_glWindow->redraw();
}

void MainWindow::handleSelectionChanged()
{
    const ccHObject::Container &selectedEntities = m_selectedEntities; //getSelectedEntities();

    const auto &list = m_stdPlugins;

    for ( ccPluginInterface *plugin : list )
    {
        if (plugin->getType() == CC_STD_PLUGIN )
        {
            ccStdPluginInterface *stdPlugin = static_cast<ccStdPluginInterface *>(plugin);

            stdPlugin->onNewSelection(selectedEntities);
        }
    }
}

void MainWindow::on_action_new_triggered()
{

}

void MainWindow::on_action_open_triggered()
{

}

void MainWindow::on_action_exit_triggered()
{
    close();
}

void MainWindow::on_action_about_triggered()
{
    m_AboutDlg->exec();
}

void MainWindow::on_action_openImage_triggered()
{

}

void MainWindow::on_action_openPointCloud_triggered()
{
    //addToDB without filters
    /*
    QStringList FileNames = QFileDialog::getOpenFileNames(this, tr(u8"打开文件"), ".", "All(*.*)");

    if (FileNames.isEmpty())
    {
        QMessageBox::information(this, tr(u8"信息"), tr(u8"请选择文件！"));
    }
    else
    {
        addToDB(FileNames);
    }
    */

    //persistent settings
    QSettings settings;
    settings.beginGroup(ccPS::LoadFile());
    QString currentPath = settings.value(ccPS::CurrentPath(), ccFileUtils::defaultDocPath()).toString();
    QString currentOpenDlgFilter = settings.value(ccPS::SelectedInputFilter(), BinFilter::GetFileFilter()).toString();

    // Add all available file I/O filters (with import capabilities)
    const QStringList filterStrings = FileIOFilter::ImportFilterList();
    const QString &allFilter = filterStrings.at( 0 );

    if ( !filterStrings.contains( currentOpenDlgFilter ) )
    {
        currentOpenDlgFilter = allFilter;
    }

    //file choosing dialog
    QStringList selectedFiles = QFileDialog::getOpenFileNames(	this,
                                                                tr(u8"打开文件(可多选)"),
                                                                currentPath,
                                                                filterStrings.join(s_fileFilterSeparator),
                                                                &currentOpenDlgFilter,
                                                                CCFileDialogOptions());
    if (selectedFiles.isEmpty())
        return;

    //save last loading parameters
    currentPath = QFileInfo(selectedFiles[0]).absolutePath();
    settings.setValue(ccPS::CurrentPath(),currentPath);
    settings.setValue(ccPS::SelectedInputFilter(),currentOpenDlgFilter);
    settings.endGroup();

    if (currentOpenDlgFilter == allFilter)
    {
        currentOpenDlgFilter.clear(); //this way FileIOFilter will try to guess the file type automatically!
    }

    //load files
    addToDBFilter(selectedFiles, currentOpenDlgFilter, m_glWindow);
}

void MainWindow::on_action_savePointCloud_triggered()
{
    if (m_selectedEntities.empty())
        return;

    ccHObject clouds("clouds");
    ccHObject meshes("meshes");
    ccHObject images("images");
    ccHObject polylines("polylines");
    ccHObject other("other");
    ccHObject otherSerializable("serializable");
    ccHObject::Container entitiesToDispatch;
    entitiesToDispatch.insert(entitiesToDispatch.begin(), m_selectedEntities.begin(), m_selectedEntities.end());
    ccHObject entitiesToSave;
    while (!entitiesToDispatch.empty())
    {
        ccHObject* child = entitiesToDispatch.back();
        entitiesToDispatch.pop_back();

        if (child->isA(CC_TYPES::HIERARCHY_OBJECT))
        {
            for (unsigned j = 0; j < child->getChildrenNumber(); ++j)
                entitiesToDispatch.push_back(child->getChild(j));
        }
        else
        {
            //we put the entity in the container corresponding to its type
            ccHObject* dest = nullptr;
            if (child->isA(CC_TYPES::POINT_CLOUD))
                dest = &clouds;
            else if (child->isKindOf(CC_TYPES::MESH))
                dest = &meshes;
            else if (child->isKindOf(CC_TYPES::IMAGE))
                dest = &images;
            else if (child->isKindOf(CC_TYPES::POLY_LINE))
                dest = &polylines;
            else if (child->isSerializable())
                dest = &otherSerializable;
            else
                dest = &other;

            assert(dest);

            //we don't want double insertions if the user has clicked both the father and child
            if (!dest->find(child->getUniqueID()))
            {
                dest->addChild(child, ccHObject::DP_NONE);
                entitiesToSave.addChild(child, ccHObject::DP_NONE);
            }
        }
    }

    bool hasCloud = (clouds.getChildrenNumber() != 0);
    bool hasMesh = (meshes.getChildrenNumber() != 0);
    bool hasImages = (images.getChildrenNumber() != 0);
    bool hasPolylines = (polylines.getChildrenNumber() != 0);
    bool hasSerializable = (otherSerializable.getChildrenNumber() != 0);
    bool hasOther = (other.getChildrenNumber() != 0);

    int stdSaveTypes =		static_cast<int>(hasCloud)
                        +	static_cast<int>(hasMesh)
                        +	static_cast<int>(hasImages)
                        +	static_cast<int>(hasPolylines)
                        +	static_cast<int>(hasSerializable);
    if (stdSaveTypes == 0)
        return;

    //we set up the right file filters, depending on the selected
    //entities type (cloud, mesh, etc.).
    QStringList fileFilters;
    {
        for (const FileIOFilter::Shared &filter : FileIOFilter::GetFilters())
        {
            bool atLeastOneExclusive = false;

            //can this filter export one or several clouds?
            bool canExportClouds = true;
            if (hasCloud)
            {
                bool isExclusive = true;
                bool multiple = false;
                canExportClouds = (filter->canSave(CC_TYPES::POINT_CLOUD, multiple, isExclusive)
                                   &&(multiple || clouds.getChildrenNumber() == 1));
                atLeastOneExclusive |= isExclusive;
            }

            //can this filter export one or several meshes?
            bool canExportMeshes = true;
            if (hasMesh)
            {
                bool isExclusive = true;
                bool multiple = false;
                canExportMeshes = (filter->canSave(CC_TYPES::MESH, multiple, isExclusive)
                                   &&(multiple || meshes.getChildrenNumber() == 1));
                atLeastOneExclusive |= isExclusive;
            }

            //can this filter export one or several polylines?
            bool canExportPolylines = true;
            if (hasPolylines)
            {
                bool isExclusive = true;
                bool multiple = false;
                canExportPolylines = (filter->canSave(CC_TYPES::POLY_LINE, multiple, isExclusive)
                                      &&(multiple || polylines.getChildrenNumber() == 1));
                atLeastOneExclusive |= isExclusive;
            }

            //can this filter export one or several images?
            bool canExportImages = true;
            if (hasImages)
            {
                bool isExclusive = true;
                bool multiple = false;
                canExportImages = (filter->canSave(CC_TYPES::IMAGE, multiple, isExclusive)
                                   &&(multiple || images.getChildrenNumber() == 1) );
                atLeastOneExclusive |= isExclusive;
            }

            //can this filter export one or several other serializable entities?
            bool canExportSerializables = true;
            if (hasSerializable)
            {
                //check if all entities have the same type
                {
                    CC_CLASS_ENUM firstClassID = otherSerializable.getChild(0)->getUniqueID();
                    for (unsigned j = 1; j < otherSerializable.getChildrenNumber(); ++j)
                    {
                        if (otherSerializable.getChild(j)->getUniqueID() != firstClassID)
                        {
                            //we add a virtual second 'stdSaveType' so as to properly handle exlusivity
                            ++stdSaveTypes;
                            break;
                        }
                    }
                }

                for (unsigned j = 0; j < otherSerializable.getChildrenNumber(); ++j)
                {
                    ccHObject* child = otherSerializable.getChild(j);
                    bool isExclusive = true;
                    bool multiple = false;
                    canExportSerializables &= (filter->canSave(child->getClassID(), multiple, isExclusive)
                                               &&(multiple || otherSerializable.getChildrenNumber() == 1));
                    atLeastOneExclusive |= isExclusive;
                }
            }

            bool useThisFilter =	canExportClouds
                                &&	canExportMeshes
                                &&	canExportImages
                                &&	canExportPolylines
                                &&	canExportSerializables
                                &&	(!atLeastOneExclusive || stdSaveTypes == 1);

            if (useThisFilter)
            {
                QStringList ff = filter->getFileFilters(false);
                for (int j = 0; j < ff.size(); ++j)
                    fileFilters.append(ff[j]);
            }
        }
    }

    //persistent settings
    QSettings settings;
    settings.beginGroup(ccPS::SaveFile());

    //default filter
    QString selectedFilter = fileFilters.first();
    if (hasCloud)
        selectedFilter = settings.value(ccPS::SelectedOutputFilterCloud(),selectedFilter).toString();
    else if (hasMesh)
        selectedFilter = settings.value(ccPS::SelectedOutputFilterMesh(), selectedFilter).toString();
    else if (hasImages)
        selectedFilter = settings.value(ccPS::SelectedOutputFilterImage(), selectedFilter).toString();
    else if (hasPolylines)
        selectedFilter = settings.value(ccPS::SelectedOutputFilterPoly(), selectedFilter).toString();

    //default output path (+ filename)
    QString currentPath = settings.value(ccPS::CurrentPath(), ccFileUtils::defaultDocPath()).toString();
    QString fullPathName = currentPath;

    if (!m_selectedEntities.empty())
    {
        //hierarchy objects have generally as name: 'filename.ext (fullpath)'
        //so we must only take the first part! (otherwise this type of name
        //with a path inside perturbs the QFileDialog a lot ;))
        QString defaultFileName(m_selectedEntities.front()->getName());
        if (m_selectedEntities.front()->isA(CC_TYPES::HIERARCHY_OBJECT))
        {
            QStringList parts = defaultFileName.split(' ', QString::SkipEmptyParts);
            if (!parts.empty())
            {
                defaultFileName = parts[0];
            }
        }
    }

    //ask the user for the output filename
    QString selectedFilename = QFileDialog::getSaveFileName(this,
                                                            tr(u8"保存文件"),
                                                            fullPathName,
                                                            fileFilters.join(s_fileFilterSeparator),
                                                            &selectedFilter,
                                                            CCFileDialogOptions());

    if (selectedFilename.isEmpty())
    {
        //process cancelled by the user
        return;
    }

    CC_FILE_ERROR result = CC_FERR_NO_ERROR;
    FileIOFilter::SaveParameters parameters;
    {
        parameters.alwaysDisplaySaveDialog = true;
        parameters.parentWidget = this;
    }

    //specific case: BIN format
    if (selectedFilter == BinFilter::GetFileFilter())
    {
        if (!m_selectedEntities.empty())
        {
            result = FileIOFilter::SaveToFile(m_selectedEntities.front(), selectedFilename, parameters, selectedFilter);
        }
        else
        {
            //we'll regroup all selected entities in a temporary group
            ccHObject tempContainer;
            ConvertToGroup(m_selectedEntities, tempContainer, ccHObject::DP_NONE);
            if (tempContainer.getChildrenNumber())
            {
                result = FileIOFilter::SaveToFile(&tempContainer, selectedFilename, parameters, selectedFilter);
            }
            else
            {
                ccLog::Warning("[I/O] None of the selected entities can be saved this way...");
                result = CC_FERR_NO_SAVE;
            }
        }
    }
    else if (entitiesToSave.getChildrenNumber() != 0)
    {
        result = FileIOFilter::SaveToFile(	entitiesToSave.getChildrenNumber() > 1 ? &entitiesToSave : entitiesToSave.getChild(0),
                                            selectedFilename,
                                            parameters,
                                            selectedFilter);
    }

    //update default filters
    if (hasCloud)
        settings.setValue(ccPS::SelectedOutputFilterCloud(),selectedFilter);
    if (hasMesh)
        settings.setValue(ccPS::SelectedOutputFilterMesh(), selectedFilter);
    if (hasImages)
        settings.setValue(ccPS::SelectedOutputFilterImage(),selectedFilter);
    if (hasPolylines)
        settings.setValue(ccPS::SelectedOutputFilterPoly(), selectedFilter);

    //we update current file path
    currentPath = QFileInfo(selectedFilename).absolutePath();
    settings.setValue(ccPS::CurrentPath(),currentPath);
    settings.endGroup();
}

void MainWindow::on_action_SetViewTop_triggered()
{
    m_glWindow->setView(CC_TOP_VIEW);
}

void MainWindow::on_action_SetViewFront_triggered()
{
    m_glWindow->setView(CC_FRONT_VIEW);
}

void MainWindow::on_action_SetViewLeft_triggered()
{
    m_glWindow->setView(CC_LEFT_VIEW);
}

void MainWindow::on_action_SetViewBack_triggered()
{
    m_glWindow->setView(CC_BACK_VIEW);
}

void MainWindow::on_action_SetViewRight_triggered()
{
    m_glWindow->setView(CC_RIGHT_VIEW);
}

void MainWindow::on_action_SetViewBottom_triggered()
{
    m_glWindow->setView(CC_BOTTOM_VIEW);
}

void MainWindow::on_action_SetViewIso1_triggered()
{
    m_glWindow->setView(CC_ISO_VIEW_1);
}

void MainWindow::on_action_SetViewIso2_triggered()
{
    m_glWindow->setView(CC_ISO_VIEW_2);
}

void MainWindow::on_action_GlobalZoom_triggered()
{
    if (m_glWindow)
        m_glWindow->zoomGlobal();
}

void MainWindow::on_action_FullScreen_triggered()
{
    if (m_glWindow)
        m_glWindow->toggleExclusiveFullScreen(true);
}

void MainWindow::on_action_SetPivotOn_triggered()
{
    if (m_glWindow)
    {
        m_glWindow->setPivotVisibility(ccGLWindow::PIVOT_ALWAYS_SHOW);
        m_glWindow->redraw();
    }
}

void MainWindow::on_action_SetPivotOff_triggered()
{
    if (m_glWindow)
    {
        m_glWindow->setPivotVisibility(ccGLWindow::PIVOT_HIDE);
        m_glWindow->redraw();
    }
}

void MainWindow::on_action_SetPivotRotationOnly_triggered()
{
    if (m_glWindow)
    {
        m_glWindow->setPivotVisibility(ccGLWindow::PIVOT_SHOW_ON_MOVE);
        m_glWindow->redraw();
    }
}

void MainWindow::doEnableGLFilter()
{
    if (!m_glWindow)
    {
        ccLog::Warning("[GL filter] No active 3D view!");
        return;
    }

    QAction *action = qobject_cast<QAction*>(sender());

    if ( action == nullptr )
    {
        Q_ASSERT( false );
        return;
    }

    ccGLPluginInterface	*plugin = action->data().value<ccGLPluginInterface *>();

    if ( plugin == nullptr )
    {
        return;
    }

    Q_ASSERT( plugin->getType() == CC_GL_FILTER_PLUGIN );

    ccGlFilter* filter = plugin->getFilter();

    if ( filter != nullptr )
    {
        if ( m_glWindow->areGLFiltersEnabled() )
        {
            m_glWindow->setGlFilter( filter );

            ccLog::Print( "Note: go to << Display > Shaders & Filters > No filter >> to disable GL filter" );
        }
        else
        {
            ccLog::Error( "GL filters not supported" );
        }
    }
    else
    {
        ccLog::Error( "Can't load GL filter (an error occurred)!" );
    }
}

void MainWindow::doDisableGLFilter()
{
    if (m_glWindow)
    {
        m_glWindow->setGlFilter(nullptr);
        m_glWindow->redraw();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (m_ppDlg) {
        int x = this->x() + this->width() - 270;
        int y = this->y() + 100;
        m_ppDlg->move(x, y);
        m_ppDlg->raise();
    }
    Q_UNUSED(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() != QEvent::WindowStateChange || !m_ppDlg)
    {
        return;
    }
    else
    {
        int x = this->x() + this->width() - 270;
        int y = this->y() + 100;
        m_ppDlg->move(x, y);
        m_ppDlg->raise();
    }

}

void MainWindow::on_action_Elevation_triggered()
{
    if (!m_glWindow->getSceneDB()) return;
    unsigned int childrenNum = m_glWindow->getSceneDB()->getChildrenNumber();

    for (unsigned int i = 0; i < childrenNum; ++i)
    {
        ccHObject* ent = m_glWindow->getSceneDB()->getChild(i);

        bool lockedVertices = false;
        ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent, &lockedVertices);

        if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD))
        {
            ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
            unsigned char dim = 2;  //(0:X, 1:Y, 2:Z)
            ccColorScale::Shared colorScale = ccColorScalesManager::GetDefaultScale();
            pc->setRGBColorByHeight(dim, colorScale);
        }
        ent->showColors(true);
    }
    m_glWindow->redraw();
}

void MainWindow::on_action_Depth_triggered()
{
    if (!m_glWindow->getSceneDB()) return;
    unsigned int childrenNum = m_glWindow->getSceneDB()->getChildrenNumber();

    for (unsigned int i = 0; i < childrenNum; ++i)
    {
        ccHObject* ent = m_glWindow->getSceneDB()->getChild(i);

        bool lockedVertices = false;
        ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent, &lockedVertices);

        if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD))
        {
            ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
            unsigned char dim = 2;  //(0:X, 1:Y, 2:Z)
            ccColorScale::Shared colorScale = ccColorScalesManager::GetDefaultScale(ccColorScalesManager::DEFAULT_SCALES::GREY);
            pc->setRGBColorByHeight(dim, colorScale);
        }
        ent->showColors(true);
    }
    m_glWindow->redraw();
}

void MainWindow::on_action_Intensity_triggered()
{
    if (!m_glWindow->getSceneDB()) return;
    unsigned int childrenNum = m_glWindow->getSceneDB()->getChildrenNumber();

    for (unsigned int i = 0; i < childrenNum; ++i)
    {
        ccHObject* ent = m_glWindow->getSceneDB()->getChild(i);
        ent->showColors(false);
    }
    m_glWindow->redraw();
}

void MainWindow::on_action_Console_triggered(bool checked)
{
    if (checked)
        CGConsoleView::getInstance()->setVisible(true);
    else
        CGConsoleView::getInstance()->setVisible(false);
}

void MainWindow::on_action_PointPicking_triggered()
{
    initPointPicking();

    if (!m_ppDlg) {
        m_ppDlg = new ccPointPropertiesDlg(m_pickingHub, this);
    }
    m_ppDlg->linkWith(m_glWindow);
    m_ppDlg->start();

    int x = this->x() + this->width() - 270;
    int y = this->y() + 100;
    m_ppDlg->move(x, y);
    m_ppDlg->raise();
    m_ppDlg->show();
}

void MainWindow::on_action_ClearAll_triggered()
{
    ccHObject* currentRoot = m_glWindow->getSceneDB();
    if (currentRoot)
    {
        m_selectedObject = nullptr;
        m_selectedEntities.clear();
        m_glWindow->setSceneDB(nullptr);
        m_glWindow->redraw();
        delete currentRoot;
        currentRoot = nullptr;
    }

    checkForLoadedEntities();
}

