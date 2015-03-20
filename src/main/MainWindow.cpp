#include "BackgroundImage.h"
#include "img/bannerapp_16.xpm"
#include "img/bannerapp_32.xpm"
#include "img/bannerapp_64.xpm"
#include "img/bannerapp_128.xpm"
#include "img/bannerapp_256.xpm"
#include "CmdCopy.h"
#include "CmdCut.h"
#include "CmdDelete.h"
#include "CmdMediator.h"
#include "CmdStackShadow.h"
#include "ColorFilter.h"
#include "Curve.h"
#include "DataKey.h"
#include "DigitizeStateContext.h"
#include "DigitAxis.xpm"
#include "DigitColorPicker.xpm"
#include "DigitCurve.xpm"
#include "DigitPointMatch.xpm"
#include "DigitSegment.xpm"
#include "DigitSelect.xpm"
#include "DlgAbout.h"
#include "DlgErrorReport.h"
#include "DlgSettingsAxesChecker.h"
#include "DlgSettingsColorFilter.h"
#include "DlgSettingsCoords.h"
#include "DlgSettingsCurveProperties.h"
#include "DlgSettingsCurves.h"
#include "DlgSettingsExport.h"
#include "DlgSettingsGridRemoval.h"
#include "DlgSettingsPointMatch.h"
#include "DlgSettingsSegments.h"
#include "DocumentSerialize.h"
#include "EngaugeAssert.h"
#include "EnumsToQt.h"
#include "ExportToFile.h"
#include "GraphicsItemType.h"
#include "GraphicsScene.h"
#include "GraphicsView.h"
#include "LoadImageFromUrl.h"
#include "Logger.h"
#include "MainWindow.h"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QDomDocument>
#include <QKeyEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QSettings>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include "QtToString.h"
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include "Settings.h"
#include "StatusBar.h"
#include "TransformationStateContext.h"
#include "Version.h"
#include "ViewPointStyle.h"
#include "ViewSegmentFilter.h"
#include "ZoomFactor.h"

// These constants are used for the menu item text AND for tooltip text
const QString DIGITIZE_ACTION_AXIS_POINT (QObject::tr ("Axis Point Tool"));
const QString DIGITIZE_ACTION_COLOR_PICKER (QObject::tr ("Color Picker Tool"));
const QString DIGITIZE_ACTION_CURVE_POINT (QObject::tr ("Curve Point Tool"));
const QString DIGITIZE_ACTION_POINT_MATCH (QObject::tr ("Point Match Tool"));
const QString DIGITIZE_ACTION_SEGMENT_POINTS (QObject::tr ("Segment Points Tool"));
const QString DIGITIZE_ACTION_SELECT (QObject::tr ("Select Tool"));

const QString EMPTY_FILENAME ("");
const QString ENGAUGE_FILENAME_DESCRIPTION ("Engauge Document");
const QString ENGAUGE_FILENAME_EXTENSION ("dig");
const QString CSV_FILENAME_EXTENSION ("csv");
const QString TSV_FILENAME_EXTENSION ("tsv");

const unsigned int MAX_RECENT_FILE_LIST_SIZE = 8;

const char *VERSION_NUMBER = "6.0";

MainWindow::MainWindow(const QString &errorReportFile,
                       QWidget *parent) :
  QMainWindow(parent),
  m_engaugeFile (EMPTY_FILENAME),
  m_currentFile (EMPTY_FILENAME),
  m_layout (0),
  m_scene (0),
  m_view (0),
  m_imageNone (0),
  m_imageUnfiltered (0),
  m_imageFiltered (0),
  m_cmdMediator (0),
  m_transformationStateContext (0)
{
  LoggerUpload::bindToMainWindow(this);

  QString initialPath = QDir::currentPath();

  setCurrentFile ("");
  createIcons();
  setWindowFlags (Qt::WindowContextHelpButtonHint);
  setWindowTitle (engaugeWindowTitle ());

  createCentralWidget();
  createActions ();
  createStatusBar ();
  createMenus ();
  createToolBars ();
  createScene ();
  createLoadImageFromUrl ();
  createStateContextDigitize ();
  createStateContextTransformation ();
  createSettingsDialogs ();
  createCommandStackShadow ();
  updateControls ();

  settingsRead ();
  setCurrentFile ("");
  setUnifiedTitleAndToolBarOnMac(true);

  installEventFilter(this);
  if (!errorReportFile.isEmpty()) {
    loadErrorReportFile(initialPath,
                        errorReportFile);
  }
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (maybeSave()) {
    settingsWrite ();
    event->accept ();
  } else {
    event->ignore ();
  }
}

CmdMediator &MainWindow::cmdMediator ()
{
  ENGAUGE_CHECK_PTR (m_cmdMediator);

  return *m_cmdMediator;
}

void MainWindow::createActions()
{
  createActionsFile ();
  createActionsEdit ();
  createActionsDigitize ();
  createActionsView ();
  createActionsSettings ();
  createActionsHelp ();
}

void MainWindow::createActionsDigitize ()
{
  QPixmap pixmapAxis (DigitAxis_xpm);
  QPixmap pixmapCurve (DigitCurve_xpm);
  QPixmap pixmapColorPicker (DigitColorPicker_xpm);
  QPixmap pixmapPointMatch (DigitPointMatch_xpm);
  QPixmap pixmapSegment (DigitSegment_xpm);
  QPixmap pixmapSelect (DigitSelect_xpm);

  QIcon iconAxis (pixmapAxis);
  QIcon iconCurve (pixmapCurve);
  QIcon iconColorPicker (pixmapColorPicker);
  QIcon iconPointMatch (pixmapPointMatch);
  QIcon iconSegment (pixmapSegment);
  QIcon iconSelect (pixmapSelect);

  m_actionDigitizeSelect = new QAction (iconSelect, DIGITIZE_ACTION_SELECT, this);
  m_actionDigitizeSelect->setShortcut (QKeySequence (tr ("Shift+F2")));
  m_actionDigitizeSelect->setCheckable (true);
  m_actionDigitizeSelect->setStatusTip (tr ("Select points on screen."));
  m_actionDigitizeSelect->setWhatsThis (tr ("Select\n\n"
                                            "Select points on the screen."));
  connect (m_actionDigitizeSelect, SIGNAL (triggered ()), this, SLOT (slotDigitizeSelect ()));

  m_actionDigitizeAxis = new QAction (iconAxis, DIGITIZE_ACTION_AXIS_POINT, this);
  m_actionDigitizeAxis->setShortcut (QKeySequence (tr ("Shift+F3")));
  m_actionDigitizeAxis->setCheckable (true);
  m_actionDigitizeAxis->setStatusTip (tr ("Digitize axis points."));
  m_actionDigitizeAxis->setWhatsThis (tr ("Digitize Axis Point\n\n"
                                          "Digitizes an axis point by placing a new point at the cursor "
                                          "after a mouse click. The coordinates of the axis point are then "
                                          "entered. In a graph, three axis points are required to define "
                                          "the graph coordinates."));
  connect (m_actionDigitizeAxis, SIGNAL (triggered ()), this, SLOT (slotDigitizeAxis ()));

  m_actionDigitizeCurve = new QAction (iconCurve, DIGITIZE_ACTION_CURVE_POINT, this);
  m_actionDigitizeCurve->setShortcut (QKeySequence (tr ("Shift+F4")));
  m_actionDigitizeCurve->setCheckable (true);
  m_actionDigitizeCurve->setStatusTip (tr ("Digitize curve points."));
  m_actionDigitizeCurve->setWhatsThis (tr ("Digitize Curve Point\n\n"
                                           "Digitizes a curve point by placing a new point at the cursor "
                                           "after a mouse click. Use this mode to digitize points along curves "
                                           "one by one.\n\n"
                                           "New points will be assigned to the currently selected curve."));
  connect (m_actionDigitizeCurve, SIGNAL (triggered ()), this, SLOT (slotDigitizeCurve ()));

  m_actionDigitizePointMatch = new QAction (iconPointMatch, DIGITIZE_ACTION_POINT_MATCH, this);
  m_actionDigitizePointMatch->setShortcut (QKeySequence (tr ("Shift+F5")));
  m_actionDigitizePointMatch->setCheckable (true);
  m_actionDigitizePointMatch->setStatusTip (tr ("Digitize curve points in a point plot by matching a point."));
  m_actionDigitizePointMatch->setWhatsThis (tr ("Digitize Curve Points by Point Matching\n\n"
                                                "Digitizes curve points in a point plot by finding points that match a sample point. The process "
                                                "starts by selecting a representative sample point.\n\n"
                                                "New points will be assigned to the currently selected curve."));
  connect (m_actionDigitizePointMatch, SIGNAL (triggered ()), this, SLOT (slotDigitizePointMatch ()));

  m_actionDigitizeColorPicker = new QAction (iconColorPicker, DIGITIZE_ACTION_COLOR_PICKER, this);
  m_actionDigitizeColorPicker->setShortcut (QKeySequence (tr ("Shift+F6")));
  m_actionDigitizeColorPicker->setCheckable (true);
  m_actionDigitizeColorPicker->setStatusTip (tr ("Select color settings for filtering in Segment Points mode."));
  m_actionDigitizeColorPicker->setWhatsThis (tr ("Select color settings for Segment Points filtering\n\n"
                                                 "Select a pixel along the currently selected curve. That pixel and its neighbors will "
                                                 "define the filter settings (color, brightness, and so on) of the currently selected curve "
                                                 "while in Segment Points mode."));
  connect (m_actionDigitizeColorPicker, SIGNAL (triggered ()), this, SLOT (slotDigitizeColorPicker ()));

  m_actionDigitizeSegment = new QAction (iconSegment, DIGITIZE_ACTION_SEGMENT_POINTS, this);
  m_actionDigitizeSegment->setShortcut (QKeySequence (tr ("Shift+F7")));
  m_actionDigitizeSegment->setCheckable (true);
  m_actionDigitizeSegment->setStatusTip (tr ("Digitize points along a segment of a curve."));
  m_actionDigitizeSegment->setWhatsThis (tr ("Digitize Segment Fill\n\n"
                                             "Digitizes a curve or segment by placing points along the "
                                             "segment under the cursor. Use this mode to quickly digitize multiple points along a "
                                             "curve with a single click.\n\n"
                                             "New points will be assigned to the currently selected curve."));
  connect (m_actionDigitizeSegment, SIGNAL (triggered ()), this, SLOT (slotDigitizeSegment ()));

  m_groupDigitize = new QActionGroup (this);
  m_groupDigitize->addAction (m_actionDigitizeSelect);
  m_groupDigitize->addAction (m_actionDigitizeAxis);
  m_groupDigitize->addAction (m_actionDigitizeCurve);
  m_groupDigitize->addAction (m_actionDigitizePointMatch);
  m_groupDigitize->addAction (m_actionDigitizeColorPicker);
  m_groupDigitize->addAction (m_actionDigitizeSegment);
}

void MainWindow::createActionsEdit ()
{
  m_actionEditUndo = new QAction(tr ("&Undo"), this);
  m_actionEditUndo->setShortcut (QKeySequence::Undo);
  m_actionEditUndo->setStatusTip (tr ("Undo the last operation."));
  m_actionEditUndo->setWhatsThis (tr ("Undo\n\n"
                                      "Undo the last operation."));
  // connect is applied when CmdMediator appears

  m_actionEditRedo = new QAction(tr ("&Redo"), this);
  m_actionEditRedo->setShortcut (QKeySequence::Redo);
  m_actionEditRedo->setStatusTip (tr ("Redo the last operation."));
  m_actionEditRedo->setWhatsThis (tr ("Redo\n\n"
                                      "Redo the last operation."));
  // connect is applied when CmdMediator appears

  m_actionEditCut = new QAction (tr ("Cut"), this);
  m_actionEditCut->setShortcut (QKeySequence::Cut);
  m_actionEditCut->setStatusTip (tr ("Cuts the selected points and copies them to the clipboard."));
  m_actionEditCut->setWhatsThis (tr ("Cut\n\n"
                                     "Cuts the selected points and copies them to the clipboard."));
  connect (m_actionEditCut, SIGNAL (triggered ()), this, SLOT (slotEditCut ()));

  m_actionEditCopy = new QAction (tr ("Copy"), this);
  m_actionEditCopy->setShortcut (QKeySequence::Copy);
  m_actionEditCopy->setStatusTip (tr ("Copies the selected points to the clipboard."));
  m_actionEditCopy->setWhatsThis (tr ("Copy\n\n"
                                     "Copies the selected points to the clipboard."));
  connect (m_actionEditCopy, SIGNAL (triggered ()), this, SLOT (slotEditCopy ()));

  m_actionEditPaste = new QAction (tr ("Paste"), this);
  m_actionEditPaste->setShortcut (QKeySequence::Paste);
  m_actionEditPaste->setStatusTip (tr ("Pastes the selected points from the clipboard."));
  m_actionEditPaste->setWhatsThis (tr ("Paste\n\n"
                                       "Pastes the selected points from the clipboard. They will be assigned to the current curve."));
  connect (m_actionEditPaste, SIGNAL (triggered ()), this, SLOT (slotEditPaste ()));

  m_actionEditDelete = new QAction (tr ("Delete"), this);
  m_actionEditDelete->setShortcut (QKeySequence::Delete);
  m_actionEditDelete->setStatusTip (tr ("Deletes the selected points, after copying them to the clipboard."));
  m_actionEditDelete->setWhatsThis (tr ("Delete\n\n"
                                        "Deletes the selected points, after copying them to the clipboard."));
  connect (m_actionEditDelete, SIGNAL (triggered ()), this, SLOT (slotEditDelete ()));
}

void MainWindow::createActionsFile ()
{
  m_actionImport = new QAction(tr ("&Import"), this);
  m_actionImport->setShortcut (tr ("Ctrl+I"));
  m_actionImport->setStatusTip (tr ("Creates a new document by importing an image."));
  m_actionImport->setWhatsThis (tr ("New Document\n\n"
                                    "Creates a new document by importing an image."));
  connect (m_actionImport, SIGNAL (triggered ()), this, SLOT (slotFileImport ()));

  m_actionOpen = new QAction(tr ("&Open"), this);
  m_actionOpen->setShortcut (QKeySequence::Open);
  m_actionOpen->setStatusTip (tr ("Opens an existing document."));
  m_actionOpen->setWhatsThis (tr ("Open Document\n\n"
                                  "Opens an existing document."));
  connect (m_actionOpen, SIGNAL (triggered ()), this, SLOT (slotFileOpen ()));

  for (unsigned int i = 0; i < MAX_RECENT_FILE_LIST_SIZE; i++) {
    QAction *recentFileAction = new QAction (this);
    recentFileAction->setVisible (true);
    connect (recentFileAction, SIGNAL (triggered ()), this, SLOT (slotRecentFileAction ()));
    m_actionRecentFiles.append (recentFileAction);
  }

  m_actionSave = new QAction(tr ("&Save"), this);
  m_actionSave->setShortcut (QKeySequence::Save);
  m_actionSave->setStatusTip (tr ("Saves the current document."));
  m_actionSave->setWhatsThis (tr ("Save Document\n\n"
                                  "Saves the current document."));
  connect (m_actionSave, SIGNAL (triggered ()), this, SLOT (slotFileSave ()));

  m_actionSaveAs = new QAction(tr ("Save As"), this);
  m_actionSaveAs->setShortcut (QKeySequence::SaveAs);
  m_actionSaveAs->setStatusTip (tr ("Saves the current document under a new filename."));
  m_actionSaveAs->setWhatsThis (tr ("Save Document As\n\n"
                                    "Saves the current document under a new filename."));
  connect (m_actionSaveAs, SIGNAL (triggered ()), this, SLOT (slotFileSaveAs ()));

  m_actionExport = new QAction (tr ("Export"), this);
  m_actionExport->setShortcut (tr ("Ctrl+E"));
  m_actionExport->setStatusTip (tr ("Exports the current document into a text file."));
  m_actionExport->setWhatsThis (tr ("Export Document\n\n"
                                    "Exports the current document into a text file."));
  connect (m_actionExport, SIGNAL (triggered ()), this, SLOT (slotFileExport ()));

  m_actionPrint = new QAction (tr ("&Print"), this);
  m_actionPrint->setShortcut (QKeySequence::Print);
  m_actionPrint->setStatusTip (tr ("Print the current document."));
  m_actionPrint->setWhatsThis (tr ("Print Document\n\n"
                                   "Print the current document to a printer or file."));
  connect (m_actionPrint, SIGNAL (triggered ()), this, SLOT (slotFilePrint ()));

  m_actionExit = new QAction(tr ("&Exit"), this);
  m_actionExit->setShortcut (QKeySequence::Quit);
  m_actionExit->setStatusTip (tr ("Quits the applicaiton."));
  m_actionExit->setWhatsThis (tr ("Exit\n\n"
                                  "Quits the applicaiton."));
  connect (m_actionExit, SIGNAL (triggered ()), this, SLOT (close ()));
}

void MainWindow::createActionsHelp ()
{
  m_actionWhatsThis = QWhatsThis::createAction(this);
  m_actionWhatsThis->setShortcut (QKeySequence::WhatsThis);

  m_actionAbout = new QAction(tr ("About Engauge"), this);
  m_actionAbout->setStatusTip (tr ("About the application."));
  m_actionAbout->setWhatsThis (tr ("About Engauge\n\nAbout the application."));
  connect (m_actionAbout, SIGNAL (triggered ()), this, SLOT (slotHelpAbout ()));
}

void MainWindow::createActionsSettings ()
{
  m_actionSettingsCoords = new QAction (tr ("Coordinates"), this);
  m_actionSettingsCoords->setStatusTip (tr ("Edit Coordinate settings."));
  m_actionSettingsCoords->setWhatsThis (tr ("Coordinate Settings\n\n"
                                            "Coordinate settings determine how the graph coordinates are mapped to the pixels in the image"));
  connect (m_actionSettingsCoords, SIGNAL (triggered ()), this, SLOT (slotSettingsCoords ()));

  m_actionSettingsCurveProperties = new QAction (tr ("Curve Properties"), this);
  m_actionSettingsCurveProperties->setStatusTip (tr ("Edit Curve Properties settings."));
  m_actionSettingsCurveProperties->setWhatsThis (tr ("Curve Properties Settings\n\n"
                                                     "Curves properties settings determine how each curve appears"));
  connect (m_actionSettingsCurveProperties, SIGNAL (triggered ()), this, SLOT (slotSettingsCurveProperties ()));

  m_actionSettingsCurves = new QAction (tr ("Curves"), this);
  m_actionSettingsCurves->setStatusTip (tr ("Edit Curves settings."));
  m_actionSettingsCurves->setWhatsThis (tr ("Curves Settings\n\n"
                                            "Curves settings determine which curves are included in the current document"));
  connect (m_actionSettingsCurves, SIGNAL (triggered ()), this, SLOT (slotSettingsCurves ()));

  m_actionSettingsExport = new QAction (tr ("Export"), this);
  m_actionSettingsExport->setStatusTip (tr ("Edit Export settings."));
  m_actionSettingsExport->setWhatsThis (tr ("Export Settings\n\n"
                                            "Export settings affect how exported files are formatted"));
  connect (m_actionSettingsExport, SIGNAL (triggered ()), this, SLOT (slotSettingsExport ()));

  m_actionSettingsColorFilter = new QAction (tr ("Color Filter"), this);
  m_actionSettingsColorFilter->setStatusTip (tr ("Edit Color Filter settings."));
  m_actionSettingsColorFilter->setWhatsThis (tr ("Color Filter Settings\n\n"
                                                 "Color filtering simplifies the graphs for easier Point Matching and Segment Filling"));
  connect (m_actionSettingsColorFilter, SIGNAL (triggered ()), this, SLOT (slotSettingsColorFilter ()));

  m_actionSettingsAxesChecker = new QAction (tr ("Axes Checker"), this);
  m_actionSettingsAxesChecker->setStatusTip (tr ("Edit Axes Checker settings."));
  m_actionSettingsAxesChecker->setWhatsThis (tr ("Axes Checker Settings\n\n"
                                                 "Axes checker can reveal any axis point mistakes, which are otherwise hard to find."));
  connect (m_actionSettingsAxesChecker, SIGNAL (triggered ()), this, SLOT (slotSettingsAxesChecker ()));

  m_actionSettingsGridRemoval = new QAction (tr ("Grid Removal"), this);
  m_actionSettingsGridRemoval->setStatusTip (tr ("Edit Grid Removal settings."));
  m_actionSettingsGridRemoval->setWhatsThis (tr ("Grid Removal Settings\n\n"
                                                 "Grid removal simplifies the graphs for easier Point Matching and Segment Filling, when "
                                                 "Color Filtering is not enough."));
  connect (m_actionSettingsGridRemoval, SIGNAL (triggered ()), this, SLOT (slotSettingsGridRemoval ()));

  m_actionSettingsPointMatch = new QAction (tr ("Point Match"), this);
  m_actionSettingsPointMatch->setStatusTip (tr ("Edit Point Match settings."));
  m_actionSettingsPointMatch->setWhatsThis (tr ("Point Match Settings\n\n"
                                                "Point match settings determine how points are matched while in Point Match mode"));
  connect (m_actionSettingsPointMatch, SIGNAL (triggered ()), this, SLOT (slotSettingsPointMatch ()));

  m_actionSettingsSegments = new QAction (tr ("Segment Fill"), this);
  m_actionSettingsSegments->setStatusTip (tr ("Edit Segment Fill settings."));
  m_actionSettingsSegments->setWhatsThis (tr ("Segment Fill Settings\n\n"
                                              "Segment fill settings determine how points are generated in the Segment Fill mode"));
  connect (m_actionSettingsSegments, SIGNAL (triggered ()), this, SLOT (slotSettingsSegments ()));
}

void MainWindow::createActionsView ()
{
  m_actionViewBackground = new QAction (tr ("Background Toolbar"), this);
  m_actionViewBackground->setCheckable (true);
  m_actionViewBackground->setChecked (true);
  m_actionViewBackground->setStatusTip (tr ("Show or hide the background toolbar."));
  m_actionViewBackground->setWhatsThis (tr ("View Background ToolBar\n\n"
                                            "Show or hide the background toolbar"));
  connect (m_actionViewBackground, SIGNAL (triggered ()), this, SLOT (slotViewToolBarBackground ()));

  m_actionViewDigitize = new QAction (tr ("Digitizing Tools Toolbar"), this);
  m_actionViewDigitize->setCheckable (true);
  m_actionViewDigitize->setChecked (true);
  m_actionViewDigitize->setStatusTip (tr ("Show or hide the digitizing tools toolbar."));
  m_actionViewDigitize->setWhatsThis (tr ("View Digitizing Tools ToolBar\n\n"
                                          "Show or hide the digitizing tools toolbar"));
  connect (m_actionViewDigitize, SIGNAL (triggered ()), this, SLOT (slotViewToolBarDigitize()));

  m_actionViewSettingsViews = new QAction (tr ("Settings Views Toolbar"), this);
  m_actionViewSettingsViews->setCheckable (true);
  m_actionViewSettingsViews->setChecked (true);
  m_actionViewSettingsViews->setStatusTip (tr ("Show or hide the settings views toolbar."));
  m_actionViewSettingsViews->setWhatsThis (tr ("View Settings Views ToolBar\n\n"
                                               "Show or hide the settings views toolbar. These views graphically show the "
                                               "most important settings."));
  connect (m_actionViewSettingsViews, SIGNAL (triggered ()), this, SLOT (slotViewToolBarSettingsViews()));

  m_actionViewToolTips = new QAction (tr ("View Tool Tips"), this);
  m_actionViewToolTips->setCheckable (true);
  m_actionViewToolTips->setChecked (true);
  m_actionViewToolTips->setStatusTip (tr ("Show or hide the tool tips."));
  m_actionViewToolTips->setWhatsThis (tr ("View Tool Tips\n\n"
                                          "Show or hide the tool tips"));
  connect (m_actionViewToolTips, SIGNAL (triggered ()), this, SLOT (slotViewToolTips()));

  m_actionViewBackgroundNone = new QAction (tr ("No Background"), this);
  m_actionViewBackgroundNone->setCheckable (true);
  m_actionViewBackgroundNone->setStatusTip (tr ("Do not show the image underneath the points."));
  m_actionViewBackgroundNone->setWhatsThis (tr ("No Background\n\n"
                                                "No image is shown so points are easier to see"));

  m_actionViewBackgroundOriginal = new QAction (tr ("Show Original Image"), this);
  m_actionViewBackgroundOriginal->setCheckable (true);
  m_actionViewBackgroundOriginal->setStatusTip (tr ("Show the original image underneath the points."));
  m_actionViewBackgroundOriginal->setWhatsThis (tr ("Show Original Image\n\n"
                                                    "Show the original image underneath the points"));

  m_actionViewBackgroundFiltered = new QAction (tr ("Show Filtered Image"), this);
  m_actionViewBackgroundFiltered->setCheckable (true);
  m_actionViewBackgroundFiltered->setChecked (true);
  m_actionViewBackgroundFiltered->setStatusTip (tr ("Show the filtered image underneath the points."));
  m_actionViewBackgroundFiltered->setWhatsThis (tr ("Show Filtered Image\n\n"
                                                    "Show the filtered image underneath the points.\n\n"
                                                    "The filtered image is created from the original image according to the "
                                                    "Filter preferences so unimportant information is hidden and important "
                                                    "information is emphasized"));

  m_actionViewPointsNone = new QAction (tr ("Hide All Points"), this);
  m_actionViewPointsNone->setCheckable (true);
  m_actionViewPointsNone->setStatusTip (tr ("Hide all digitized points."));
  m_actionViewPointsNone->setWhatsThis (tr ("Hide All Points\n\n"
                                            "No digitized points are shown so the image is easier to see."));

  m_actionViewPointsCurve = new QAction (tr ("Show Selected Curve's Points"), this);
  m_actionViewPointsCurve->setCheckable (true);
  m_actionViewPointsCurve->setStatusTip (tr ("Show only points in the currently selected curve."));
  m_actionViewPointsCurve->setWhatsThis (tr ("Show Curve's Points\n\n"
                                             "Show only digitized points that belong to the currently selected curve."));

  m_actionViewPointsAll = new QAction (tr ("Show All Points"), this);
  m_actionViewPointsAll->setCheckable (true);
  m_actionViewPointsAll->setChecked (true);
  m_actionViewPointsAll->setStatusTip (tr ("Show all points."));
  m_actionViewPointsAll->setWhatsThis (tr ("Show All Points\n\n"
                                           "Show all digitized axis and curve points"));

  m_groupBackground = new QActionGroup(this);
  m_groupBackground->addAction (m_actionViewBackgroundNone);
  m_groupBackground->addAction (m_actionViewBackgroundOriginal);
  m_groupBackground->addAction (m_actionViewBackgroundFiltered);
  connect (m_groupBackground, SIGNAL(triggered (QAction*)), this, SLOT (slotViewGroupBackground(QAction*)));

  m_groupPoints = new QActionGroup(this);
  m_groupPoints->addAction (m_actionViewPointsNone);
  m_groupPoints->addAction (m_actionViewPointsCurve);
  m_groupPoints->addAction (m_actionViewPointsAll);
  connect (m_groupPoints, SIGNAL(triggered (QAction*)), this, SLOT (slotViewGroupPoints(QAction*)));

  m_actionStatusNever = new QAction (tr ("Hide Always"), this);
  m_actionStatusNever->setCheckable(true);
  m_actionStatusNever->setStatusTip (tr ("Always hide the status bar."));
  m_actionStatusNever->setWhatsThis (tr ("Hide the status bar. No temporary status or feedback messages will appear."));

  m_actionStatusTemporary = new QAction (tr ("Show Temporary Messages"), this);
  m_actionStatusTemporary->setCheckable(true);
  m_actionStatusTemporary->setStatusTip (tr ("Hide the status bar except when display temporary messages."));
  m_actionStatusTemporary->setWhatsThis (tr ("Hide the status bar, except when displaying temporary status and feedback messages."));

  m_actionStatusAlways = new QAction (tr ("Show Always"), this);
  m_actionStatusAlways->setCheckable(true);
  m_actionStatusAlways->setStatusTip (tr ("Always show the status bar."));
  m_actionStatusAlways->setWhatsThis (tr ("Show the status bar. Besides displaying temporary status and feedback messages, "
                                          "the status bar also displays information about the cursor position."));

  m_groupStatus = new QActionGroup(this);
  m_groupStatus->addAction (m_actionStatusNever);
  m_groupStatus->addAction (m_actionStatusTemporary);
  m_groupStatus->addAction (m_actionStatusAlways);
  connect (m_groupStatus, SIGNAL (triggered (QAction*)), this, SLOT (slotViewGroupStatus(QAction*)));

  m_actionZoomOut = new QAction (tr ("Zoom Out"), this);
  m_actionZoomOut->setStatusTip (tr ("Zoom out"));
  m_actionZoomOut->setShortcut (tr ("-"));
  connect (m_actionZoomOut, SIGNAL (triggered ()), this, SLOT (slotViewZoomOut ()));

  m_actionZoomIn = new QAction (tr ("Zoom In"), this);
  m_actionZoomIn->setStatusTip (tr ("Zoom in"));
  m_actionZoomIn->setShortcut (tr ("+"));
  connect (m_actionZoomIn, SIGNAL (triggered ()), this, SLOT (slotViewZoomIn ()));

  m_actionZoom16To1 = new QAction (tr ("16:1 (1600%)"), this);
  m_actionZoom16To1->setCheckable (true);
  m_actionZoom16To1->setStatusTip (tr ("Zoom 16:1"));
  connect (m_actionZoom16To1, SIGNAL (triggered ()), this, SLOT (slotViewZoom16To1 ()));

  m_actionZoom8To1 = new QAction (tr ("8:1 (800%)"), this);
  m_actionZoom8To1->setCheckable (true);
  m_actionZoom8To1->setStatusTip (tr ("Zoom 8:1"));
  connect (m_actionZoom8To1, SIGNAL (triggered ()), this, SLOT (slotViewZoom8To1 ()));

  m_actionZoom4To1 = new QAction (tr ("4:1 (400%)"), this);
  m_actionZoom4To1->setCheckable (true);
  m_actionZoom4To1->setStatusTip (tr ("Zoom 4:1"));
  connect (m_actionZoom4To1, SIGNAL (triggered ()), this, SLOT (slotViewZoom4To1 ()));

  m_actionZoom2To1 = new QAction (tr ("2:1 (200%)"), this);
  m_actionZoom2To1->setCheckable (true);
  m_actionZoom2To1->setStatusTip (tr ("Zoom 2:1"));
  connect (m_actionZoom2To1, SIGNAL (triggered ()), this, SLOT (slotViewZoom2To1 ()));

  m_actionZoom1To1 = new QAction (tr ("1:1 (100%)"), this);
  m_actionZoom1To1->setCheckable (true);
  m_actionZoom1To1->setChecked (true);
  m_actionZoom1To1->setStatusTip (tr ("Zoom 6:1"));
  connect (m_actionZoom1To1, SIGNAL (triggered ()), this, SLOT (slotViewZoom1To1 ()));

  m_actionZoom1To2 = new QAction (tr ("1:2 (50%)"), this);
  m_actionZoom1To2->setCheckable (true);
  m_actionZoom1To2->setStatusTip (tr ("Zoom 1:2"));
  connect (m_actionZoom1To2, SIGNAL (triggered ()), this, SLOT (slotViewZoom1To2 ()));

  m_actionZoom1To4 = new QAction (tr ("1:4 (25%)"), this);
  m_actionZoom1To4->setCheckable (true);
  m_actionZoom1To4->setStatusTip (tr ("Zoom 1:4"));
  connect (m_actionZoom1To4, SIGNAL (triggered ()), this, SLOT (slotViewZoom1To4 ()));

  m_actionZoom1To8 = new QAction (tr ("1:8 (12.5%)"), this);
  m_actionZoom1To8->setCheckable (true);
  m_actionZoom1To8->setStatusTip (tr ("Zoom 1:8"));
  connect (m_actionZoom1To8, SIGNAL (triggered ()), this, SLOT (slotViewZoom1To8 ()));

  m_actionZoom1To16 = new QAction (tr ("1:16 (6.25%)"), this);
  m_actionZoom1To16->setCheckable (true);
  m_actionZoom1To16->setStatusTip (tr ("Zoom 1:16"));
  connect (m_actionZoom1To16, SIGNAL (triggered ()), this, SLOT (slotViewZoom1To16 ()));

  m_actionZoomFill = new QAction (tr ("Fill"), this);
  m_actionZoomFill->setCheckable (true);
  m_actionZoomFill->setStatusTip (tr ("Zoom with stretching to fill window"));
  connect (m_actionZoomFill, SIGNAL (triggered ()), this, SLOT (slotViewZoomFill ()));

  m_groupZoom = new QActionGroup (this);
  m_groupZoom->addAction (m_actionZoom16To1);
  m_groupZoom->addAction (m_actionZoom8To1);
  m_groupZoom->addAction (m_actionZoom4To1);
  m_groupZoom->addAction (m_actionZoom2To1);
  m_groupZoom->addAction (m_actionZoom1To1);
  m_groupZoom->addAction (m_actionZoom1To2);
  m_groupZoom->addAction (m_actionZoom1To4);
  m_groupZoom->addAction (m_actionZoom1To8);
  m_groupZoom->addAction (m_actionZoom1To16);
  m_groupZoom->addAction (m_actionZoomFill);
}

void MainWindow::createCentralWidget ()
{
  QWidget *widget = new QWidget;
  setCentralWidget (widget);
  m_layout = new QVBoxLayout;
  widget->setLayout (m_layout);
}

void MainWindow::createCommandStackShadow ()
{
  m_cmdStackShadow = new CmdStackShadow;
}

void MainWindow::createIcons()
{
  QIcon icon;
  QPixmap icon16 (bannerapp_16);
  QPixmap icon32 (bannerapp_32);
  QPixmap icon64 (bannerapp_64);
  QPixmap icon128 (bannerapp_128);
  QPixmap icon256 (bannerapp_256);

  icon.addPixmap (icon16);
  icon.addPixmap (icon32);
  icon.addPixmap (icon64);
  icon.addPixmap (icon128);
  icon.addPixmap (icon256);

  setWindowIcon (icon);
}

void MainWindow::createLoadImageFromUrl ()
{
  m_loadImageFromUrl = new LoadImageFromUrl (*this);
}

void MainWindow::createMenus()
{
  m_menuFile = menuBar()->addMenu(tr("&File"));
  m_menuFile->addAction (m_actionImport);
  m_menuFile->addAction (m_actionOpen);
  m_menuFileOpenRecent = new QMenu (tr ("Open &Recent"));
  for (unsigned int i = 0; i < MAX_RECENT_FILE_LIST_SIZE; i++) {
    m_menuFileOpenRecent->addAction (m_actionRecentFiles.at (i));
  }
  m_menuFile->addMenu (m_menuFileOpenRecent);
  m_menuFile->insertSeparator (m_actionSave);
  m_menuFile->addAction (m_actionSave);
  m_menuFile->addAction (m_actionSaveAs);
  m_menuFile->addAction (m_actionExport);
  m_menuFile->insertSeparator (m_actionPrint);
  m_menuFile->addAction (m_actionPrint);
  m_menuFile->insertSeparator (m_actionExit);
  m_menuFile->addAction (m_actionExit);

  m_menuEdit = menuBar()->addMenu(tr("&Edit"));
  m_menuEdit->addAction (m_actionEditUndo);
  m_menuEdit->addAction (m_actionEditRedo);
  m_menuEdit->insertSeparator (m_actionEditCut);
  m_menuEdit->addAction (m_actionEditCut);
  m_menuEdit->addAction (m_actionEditCopy);
  m_menuEdit->addAction (m_actionEditPaste);
  m_menuEdit->addAction (m_actionEditDelete);

  m_menuDigitize = menuBar()->addMenu(tr("Digitize"));
  m_menuDigitize->addAction (m_actionDigitizeSelect);
  m_menuDigitize->addAction (m_actionDigitizeAxis);
  m_menuDigitize->addAction (m_actionDigitizeCurve);
  m_menuDigitize->addAction (m_actionDigitizePointMatch);
  m_menuDigitize->addAction (m_actionDigitizeColorPicker);
  m_menuDigitize->addAction (m_actionDigitizeSegment);

  m_menuView = menuBar()->addMenu(tr("View"));
  m_menuView->addAction (m_actionViewBackground);
  m_menuView->addAction (m_actionViewDigitize);
  m_menuView->addAction (m_actionViewSettingsViews);
  m_menuView->insertSeparator (m_actionViewToolTips);
  m_menuView->addAction (m_actionViewToolTips);
  m_menuView->insertSeparator (m_actionViewBackgroundNone);
  m_menuViewBackground = new QMenu (tr ("Background"));
  m_menuViewBackground->addAction (m_actionViewBackgroundNone);
  m_menuViewBackground->addAction (m_actionViewBackgroundOriginal);
  m_menuViewBackground->addAction (m_actionViewBackgroundFiltered);
  m_menuView->addMenu (m_menuViewBackground);
  m_menuViewPoints = new QMenu (tr ("Points"));
  m_menuViewPoints->addAction (m_actionViewPointsNone);
  m_menuViewPoints->addAction (m_actionViewPointsCurve);
  m_menuViewPoints->addAction (m_actionViewPointsAll);
  m_menuView->addMenu (m_menuViewPoints);
  m_menuViewStatus = new QMenu (tr ("Status Bar"));
  m_menuViewStatus->addAction (m_actionStatusNever);
  m_menuViewStatus->addAction (m_actionStatusTemporary);
  m_menuViewStatus->addAction (m_actionStatusAlways);
  m_menuView->addMenu (m_menuViewStatus);
  m_menuViewZoom = new QMenu (tr ("Zoom"));
  m_menuViewZoom->addAction (m_actionZoomOut);
  m_menuViewZoom->addAction (m_actionZoomIn);
  m_menuViewZoom->insertSeparator (m_actionZoom16To1);
  m_menuViewZoom->addAction (m_actionZoom16To1);
  m_menuViewZoom->addAction (m_actionZoom8To1);
  m_menuViewZoom->addAction (m_actionZoom4To1);
  m_menuViewZoom->addAction (m_actionZoom2To1);
  m_menuViewZoom->addAction (m_actionZoom1To1);
  m_menuViewZoom->addAction (m_actionZoom1To2);
  m_menuViewZoom->addAction (m_actionZoom1To4);
  m_menuViewZoom->addAction (m_actionZoom1To8);
  m_menuViewZoom->addAction (m_actionZoom1To16);
  m_menuViewZoom->addAction (m_actionZoomFill);
  m_menuView->addMenu (m_menuViewZoom);

  m_menuSettings = menuBar()->addMenu(tr ("Settings"));
  m_menuSettings->addAction (m_actionSettingsCoords);
  m_menuSettings->addAction (m_actionSettingsCurveProperties);
  m_menuSettings->addAction (m_actionSettingsCurves);
  m_menuSettings->addAction (m_actionSettingsExport);
  m_menuSettings->addAction (m_actionSettingsColorFilter);
  m_menuSettings->addAction (m_actionSettingsAxesChecker);
  m_menuSettings->addAction (m_actionSettingsGridRemoval);
  m_menuSettings->addAction (m_actionSettingsPointMatch);
  m_menuSettings->addAction (m_actionSettingsSegments);

  m_menuHelp = menuBar()->addMenu(tr("&Help"));
  m_menuHelp->addAction (m_actionWhatsThis);
  m_menuHelp->insertSeparator (m_actionAbout);
  m_menuHelp->addAction (m_actionAbout);

  updateRecentFileList();
}

void MainWindow::createSettingsDialogs ()
{
  m_dlgSettingsCoords = new DlgSettingsCoords (*this);
  m_dlgSettingsCurveProperties = new DlgSettingsCurveProperties (*this);
  m_dlgSettingsCurves = new DlgSettingsCurves (*this);
  m_dlgSettingsExport = new DlgSettingsExport (*this);
  m_dlgSettingsColorFilter = new DlgSettingsColorFilter (*this);
  m_dlgSettingsAxesChecker = new DlgSettingsAxesChecker (*this);
  m_dlgSettingsGridRemoval = new DlgSettingsGridRemoval (*this);
  m_dlgSettingsPointMatch = new DlgSettingsPointMatch (*this);
  m_dlgSettingsSegments = new DlgSettingsSegments (*this);

  m_dlgSettingsCoords->setVisible (false);
  m_dlgSettingsCurveProperties->setVisible (false);
  m_dlgSettingsCurves->setVisible (false);
  m_dlgSettingsExport->setVisible (false);
  m_dlgSettingsColorFilter->setVisible (false);
  m_dlgSettingsAxesChecker->setVisible (false);
  m_dlgSettingsGridRemoval->setVisible (false);
  m_dlgSettingsPointMatch->setVisible (false);
  m_dlgSettingsSegments->setVisible (false);
}

void MainWindow::createScene ()
{
  m_scene = new GraphicsScene (this);
  m_view = new GraphicsView (m_scene, *this);
  m_layout->addWidget (m_view);
}

void MainWindow::createStateContextDigitize ()
{
  m_digitizeStateContext = new DigitizeStateContext (*this,
                                                     *m_view);
}

void MainWindow::createStateContextTransformation ()
{
  ENGAUGE_CHECK_PTR (m_scene);

  m_transformationStateContext = new TransformationStateContext (*m_scene);
}

void MainWindow::createStatusBar ()
{
  m_statusBar = new StatusBar (*statusBar ());
  connect (this, SIGNAL (signalZoom(int)), m_statusBar, SLOT (slotZoom(int)));
  connect (m_statusBar, SIGNAL (signalZoom (int)), this, SLOT (slotViewZoom (int)));
}

void MainWindow::createToolBars ()
{
  const int VIEW_SIZE = 22;

  // Background toolbar widgets
  m_cmbBackground = new QComboBox ();
  m_cmbBackground->setEnabled (false);
  m_cmbBackground->setStatusTip (tr ("Select background image"));
  m_cmbBackground->setWhatsThis (tr ("Selected Background\n\n"
                                     "Select background image:\n"
                                     "1) No background which highlights points\n"
                                     "2) Original image which shows everything\n"
                                     "3) Filtered image which highlights important details"));
  m_cmbBackground->addItem ("No background", QVariant (BACKGROUND_IMAGE_NONE));
  m_cmbBackground->addItem ("Original image", QVariant (BACKGROUND_IMAGE_ORIGINAL));
  m_cmbBackground->addItem ("Filtered image", QVariant (BACKGROUND_IMAGE_FILTERED));
  // selectBackgroundOriginal needs currentIndexChanged
  connect (m_cmbBackground, SIGNAL (currentIndexChanged (int)), this, SLOT (slotCmbBackground (int)));

  // Background toolbar
  m_toolBackground = new QToolBar (tr ("Background"), this);
  m_toolBackground->addWidget (m_cmbBackground);
  addToolBar (m_toolBackground);

  // Digitize toolbar widgets that are not created elsewhere
  m_cmbCurve = new QComboBox ();
  m_cmbCurve->setEnabled (false);
  m_cmbCurve->setMinimumWidth (180);
  m_cmbCurve->setStatusTip (tr ("Select curve for new points."));
  m_cmbCurve->setWhatsThis (tr ("Selected Curve Name\n\n"
                                "Select curve for any new points. Every point belongs to one curve."));
  connect (m_cmbCurve, SIGNAL (activated (int)), this, SLOT (slotCmbCurve (int))); // activated() ignores code changes

  // Digitize toolbar
  m_toolDigitize = new QToolBar (tr ("Drawing"), this);
  m_toolDigitize->addAction (m_actionDigitizeSelect);
  m_toolDigitize->insertSeparator (m_actionDigitizeAxis);
  m_toolDigitize->addAction (m_actionDigitizeAxis);
  m_toolDigitize->insertSeparator (m_actionDigitizeCurve);
  m_toolDigitize->addAction (m_actionDigitizeCurve);
  m_toolDigitize->addAction (m_actionDigitizePointMatch);
  m_toolDigitize->addAction (m_actionDigitizeColorPicker);
  m_toolDigitize->addAction (m_actionDigitizeSegment);
  m_toolDigitize->addWidget (m_cmbCurve);
  addToolBar (m_toolDigitize);

  // Views toolbar widgets
  m_viewPointStyle = new ViewPointStyle();
  m_viewPointStyle->setMinimumSize(VIEW_SIZE, VIEW_SIZE);
  m_viewPointStyle->setMaximumSize(VIEW_SIZE, VIEW_SIZE);
  m_viewPointStyle->setStatusTip (tr ("Points style for the currently selected curve"));
  m_viewPointStyle->setWhatsThis (tr ("Points Style\n\n"
                                      "Points style for the currently selected curve. The points style is only "
                                      "displayed in this toolbar. To change the points style, "
                                      "use the Curve Properties dialog."));

  m_viewSegmentFilter = new ViewSegmentFilter();
  m_viewSegmentFilter->setMinimumSize(VIEW_SIZE, VIEW_SIZE);
  m_viewSegmentFilter->setMaximumSize(VIEW_SIZE, VIEW_SIZE);
  m_viewSegmentFilter->setStatusTip (tr ("View of filter for current curve in Segment Points mode"));
  m_viewSegmentFilter->setWhatsThis (tr ("Segment Points Filter\n\n"
                                         "View of filter for the current curve in Segment Points mode. The filter settings are only "
                                         "displayed in this toolbar. To changed the filter settings, "
                                         "use the Color Picker mode or the Filter Settings dialog."));

  // Settings views toolbar
  m_toolSettingsViews = new QToolBar (tr ("Views"), this);
  m_toolSettingsViews->addWidget (m_viewPointStyle);
  m_toolSettingsViews->addWidget (new QLabel (" ")); // A hack, but this works to put some space between the adjacent widgets
  m_toolSettingsViews->addWidget (m_viewSegmentFilter);
  addToolBar (m_toolSettingsViews);
}

bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
  if (event->type () == QEvent::KeyPress) {

    QKeyEvent *eventKeyPress = (QKeyEvent *) event;

    if ((eventKeyPress->key() == Qt::Key_E) &&
        ((eventKeyPress->modifiers() & Qt::ShiftModifier) != 0) &&
        ((eventKeyPress->modifiers() & Qt::ControlModifier) != 0)) {

      saveErrorReportFileAndExit ("Shift+Control+E",
                                  __FILE__,
                                  __LINE__,
                                  "assert");
    }
  }

  return QObject::eventFilter (target, event);
}

void MainWindow::fileImport (const QString &fileName)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::fileImport fileName=" << fileName.toLatin1 ().data ();

  QString originalFileOld = m_originalFile;
  bool originalFileWasImported = m_originalFileWasImported;

  m_originalFile = fileName; // Make this available for logging in case an error occurs during the load
  m_originalFileWasImported = true;

  QImage image;
  if (!image.load (fileName)) {
    QMessageBox::warning (this,
                          engaugeWindowTitle(),
                          tr("Cannot read file %1.").
                          arg(fileName));

    // Reset
    m_originalFile = originalFileOld;
    m_originalFileWasImported = originalFileWasImported;

    return;
  }

  loadImage (fileName,
             image);
}

void MainWindow::loadCurveListFromCmdMediator ()
{
  m_cmbCurve->clear ();
  QStringList curvesGraphsNames = m_cmdMediator->curvesGraphsNames ();
  QStringList::iterator itr;
  for (itr = curvesGraphsNames.begin (); itr != curvesGraphsNames.end (); itr++) {

    QString curvesGraphName = *itr;
    m_cmbCurve->addItem (curvesGraphName);
  }
  m_cmbCurve->setCurrentIndex (0);
}

void MainWindow::loadDocumentFile (const QString &fileName)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::loadDocumentFile fileName=" << fileName.toLatin1 ().data ();

  QApplication::setOverrideCursor(Qt::WaitCursor);
  CmdMediator *cmdMediator = new CmdMediator (*this,
                                              fileName);
  QApplication::restoreOverrideCursor();

  if (cmdMediator->successfulRead ()) {

    setCurrentPathFromFile (fileName);
    rebuildRecentFileListForCurrentFile(fileName);
    m_currentFile = fileName; // This enables the FileSaveAs menu option

    if (m_cmdMediator != 0) {
      removePixmaps ();
      delete m_cmdMediator;
      m_cmdMediator = 0;
    }

    m_cmdMediator = cmdMediator;
    setupAfterLoad(fileName,
                   "File opened");

    // Start select mode
    m_actionDigitizeSelect->setChecked (true); // We assume user wants to first select existing stuff
    slotDigitizeSelect(); // Trigger transition so cursor gets updated immediately

    m_engaugeFile = fileName;
    m_originalFile = fileName; // This is needed by updateAfterCommand below if an error report is generated
    m_originalFileWasImported = false;

    updateAfterCommand (); // Enable Save button now that m_engaugeFile is set

  } else {

    QMessageBox::warning (this,
                          engaugeWindowTitle(),
                          tr("Cannot read file %1:\n%2.").
                          arg(fileName).
                          arg(cmdMediator->reasonForUnsuccessfulRead ()));
    delete cmdMediator;

  }
}

void MainWindow::loadErrorReportFile(const QString &initialPath,
                                     const QString &errorReportFile)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::loadErrorReportFile"
                              << " path=" << initialPath.toLatin1().data()
                              << " file=" << errorReportFile.toLatin1().data();

  // The default path has been changed from its original executable-based initial value to the last directory used
  // according to the settings. Since the executable-based directory is much more stable, and we want a predictable
  // directory in the likely event that the error report file has a relative path, we temporarily switch the default path
  // back to the executable-based initial value
  QString originalPath = QDir::currentPath();
  QDir::setCurrent(initialPath);

  QFile file (errorReportFile);
  if (!file.exists()) {
    // Convert path from relative to absolute so file-not-found errors are easier to fix
    QFileInfo fileInfo (errorReportFile);

    QMessageBox::critical (this,
                           engaugeWindowTitle(),
                           tr ("File not found: ") + fileInfo.absoluteFilePath());
    exit (-1);
  }

  // Open the error report file as if it was a regular Document file
  QXmlStreamReader reader (&file);
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  m_cmdMediator = new CmdMediator(*this,
                                  errorReportFile);

  // Load the commands into the shadow command stack
  m_cmdStackShadow->loadCommands (*this,
                                  m_cmdMediator->document(),
                                  reader);
  file.close();

  // Reset the original path now that the error report file has been read in
  QDir::setCurrent(originalPath);

  setupAfterLoad(errorReportFile,
                 "Error report opened");

  // Start select mode
  m_actionDigitizeSelect->setChecked (true); // We assume user wants to first select existing stuff
  slotDigitizeSelect(); // Trigger transition so cursor gets updated immediately

  updateAfterCommand ();
}

void MainWindow::loadImage (const QString &fileName,
                            const QImage &image)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::loadImage fileName=" << fileName.toLatin1 ().data ();

  QApplication::setOverrideCursor(Qt::WaitCursor);
  CmdMediator *cmdMediator = new CmdMediator (*this,
                                              image);
  QApplication::restoreOverrideCursor();

  setCurrentPathFromFile (fileName);
  // We do not call rebuildRecentFileListForCurrentFile for an image file, so only proper Engauge document files appear in the recent file list

  if (m_cmdMediator != 0) {
    removePixmaps ();
    delete m_cmdMediator;
    m_cmdMediator = 0;
  }

  m_cmdMediator = cmdMediator;
  setupAfterLoad(fileName,
                 "File imported");

  // Start axis mode
  m_actionDigitizeAxis->setChecked (true); // We assume user first wants to digitize axis points
  slotDigitizeAxis (); // Trigger transition so cursor gets updated immediately

  updateControls ();
}

void MainWindow::loadInputFileForErrorReport(QDomDocument &domInputFile) const
{
  QFile file (m_originalFile);

  // File should be available for opening, if not then the dom will be left empty. We assume it has not been
  // modified since opened
  if (!file.open (QIODevice::ReadOnly)) {
    return;
  }

  domInputFile.setContent (&file);
  file.close();
}

void MainWindow::loadToolTips()
{
  if (m_actionViewToolTips->isChecked ()) {

    // Show tool tips
    m_actionDigitizeSelect->setToolTip (DIGITIZE_ACTION_SELECT);
    m_actionDigitizeAxis->setToolTip (DIGITIZE_ACTION_AXIS_POINT);
    m_actionDigitizeCurve->setToolTip (DIGITIZE_ACTION_CURVE_POINT);
    m_actionDigitizePointMatch->setToolTip (DIGITIZE_ACTION_POINT_MATCH);
    m_actionDigitizeColorPicker->setToolTip (DIGITIZE_ACTION_COLOR_PICKER);
    m_actionDigitizeSegment->setToolTip (DIGITIZE_ACTION_SEGMENT_POINTS);
    m_cmbBackground->setToolTip (tr ("Background image."));
    m_cmbCurve->setToolTip (tr ("Currently selected curve."));
    m_viewPointStyle->setToolTip (tr ("Point style for currently selected curve."));
    m_viewSegmentFilter->setToolTip (tr ("Segment Points filter for currently selected curve."));

  } else {

    // Remove any previous tool tips
    m_actionDigitizeSelect->setToolTip ("");
    m_actionDigitizeAxis->setToolTip ("");
    m_actionDigitizeCurve->setToolTip ("");
    m_actionDigitizePointMatch->setToolTip ("");
    m_actionDigitizeColorPicker->setToolTip ("");
    m_actionDigitizeSegment->setToolTip ("");
    m_cmbBackground->setToolTip ("");
    m_cmbCurve->setToolTip ("");
    m_viewPointStyle->setToolTip ("");
    m_viewSegmentFilter->setToolTip ("");

  }
}

bool MainWindow::maybeSave()
{
  if (m_cmdMediator != 0) {
    if (m_cmdMediator->isModified()) {
      QMessageBox::StandardButton ret = QMessageBox::warning (this,
                                                              engaugeWindowTitle(),
                                                              tr("The document has been modified.\n"
                                                                 "Do you want to save your changes?"),
                                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      if (ret == QMessageBox::Save) {
        return slotFileSave();
      } else if (ret == QMessageBox::Cancel) {
        return false;
      }
    }
  }

  return true;
}

void MainWindow::rebuildRecentFileListForCurrentFile(const QString &filePath)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::rebuildRecentFileListForCurrentFile";

  setWindowFilePath (filePath);

  QSettings settings (SETTINGS_ENGAUGE, SETTINGS_DIGITIZER);
  QStringList recentFilePaths = settings.value (SETTINGS_RECENT_FILE_LIST).toStringList();
  recentFilePaths.removeAll (filePath); // Remove previous instance of the current filePath
  recentFilePaths.prepend (filePath); // Insert current filePath at start
  while (recentFilePaths.count () > (int) MAX_RECENT_FILE_LIST_SIZE) {
    recentFilePaths.removeLast (); // Remove entry since the number of entries exceeds the limit
  }
  settings.setValue (SETTINGS_RECENT_FILE_LIST, recentFilePaths);

  updateRecentFileList();
}

void MainWindow::removePixmaps ()
{
  if (m_imageNone != 0) {
    m_scene->removeItem (m_imageNone);
    m_imageNone = 0;
  }

  if (m_imageUnfiltered != 0) {
    m_scene->removeItem (m_imageUnfiltered);
    m_imageUnfiltered = 0;
  }

  if (m_imageFiltered != 0) {
    m_scene->removeItem (m_imageFiltered);
    m_imageFiltered = 0;
  }
}

void MainWindow::resizeEvent(QResizeEvent * /* event */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::resizeEvent";

  if (m_actionZoomFill->isChecked ()) {
    slotViewZoomFill();
  }
}

bool MainWindow::saveDocumentFile (const QString &fileName)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::saveDocumentFile fileName=" << fileName.toLatin1 ().data ();

  QFile file(fileName);
  if (!file.open(QFile::WriteOnly)) {
    QMessageBox::warning (this,
                          engaugeWindowTitle(),
                          tr ("Cannot write file %1: \n%2.").
                          arg(fileName).
                          arg(file.errorString()));
    return false;
  }

  rebuildRecentFileListForCurrentFile (fileName);

  QApplication::setOverrideCursor (Qt::WaitCursor);
  QXmlStreamWriter stream(&file);
  stream.setAutoFormatting(true);
  m_cmdMediator->document().saveXml(stream);
  QApplication::restoreOverrideCursor ();

  // Notify the undo stack that the current state is now considered "clean". This will automatically trigger a
  // signal back to this class that will update the modified marker in the title bar
  m_cmdMediator->setClean ();

  setCurrentFile(fileName);
  m_engaugeFile = fileName;
  updateAfterCommand (); // Enable Save button now that m_engaugeFile is set
  m_statusBar->showTemporaryMessage("File saved");

  return true;
}

void MainWindow::saveErrorReportFileAndExit (const char *context,
                                             const char *file,
                                             int line,
                                             const char *comment) const
{
  const bool DEEP_COPY = true;

  if (m_cmdMediator != 0) {

    QString xmlErrorReport;
    QXmlStreamWriter writer (&xmlErrorReport);
    writer.setAutoFormatting(true);

    // Entire error report contains metadata, commands and other details
    writer.writeStartElement(DOCUMENT_SERIALIZE_ERROR_REPORT);

    // Version
    writer.writeStartElement(DOCUMENT_SERIALIZE_APPLICATION);
    writer.writeAttribute(DOCUMENT_SERIALIZE_APPLICATION_VERSION_NUMBER, VERSION_NUMBER);
    writer.writeEndElement();

    // Operating system
    writer.writeStartElement(DOCUMENT_SERIALIZE_OPERATING_SYSTEM);
    writer.writeAttribute(DOCUMENT_SERIALIZE_OPERATING_SYSTEM_ENDIAN, EndianToString (QSysInfo::ByteOrder));
    writer.writeAttribute(DOCUMENT_SERIALIZE_OPERATING_SYSTEM_WORD_SIZE, QString::number (QSysInfo::WordSize));
    writer.writeEndElement();

    // Image
    writer.writeStartElement(DOCUMENT_SERIALIZE_IMAGE);
    writer.writeAttribute(DOCUMENT_SERIALIZE_IMAGE_WIDTH, QString::number (m_cmdMediator->pixmap().width ()));
    writer.writeAttribute(DOCUMENT_SERIALIZE_IMAGE_HEIGHT, QString::number (m_cmdMediator->pixmap().height ()));
    writer.writeEndElement();

    // Placeholder for original file, before the commands in the command stack were applied
    writer.writeStartElement(DOCUMENT_SERIALIZE_FILE);
    writer.writeAttribute(DOCUMENT_SERIALIZE_FILE_IMPORTED,
                          m_originalFileWasImported ? DOCUMENT_SERIALIZE_BOOL_TRUE : DOCUMENT_SERIALIZE_BOOL_FALSE);
    writer.writeEndElement();

    // Commands
    m_cmdMediator->saveXml(writer);

    // Error
    writer.writeStartElement(DOCUMENT_SERIALIZE_ERROR);
    writer.writeAttribute(DOCUMENT_SERIALIZE_ERROR_CONTEXT, context);
    writer.writeAttribute(DOCUMENT_SERIALIZE_ERROR_FILE, file);
    writer.writeAttribute(DOCUMENT_SERIALIZE_ERROR_LINE, QString::number (line));
    writer.writeAttribute(DOCUMENT_SERIALIZE_ERROR_COMMENT, comment);
    writer.writeEndElement();

    writer.writeEndElement();

    // Put string into DOM
    QDomDocument domErrorReport ("ErrorReport");
    domErrorReport.setContent (xmlErrorReport);

    // Postprocessing
    if (!m_originalFileWasImported) {

      // Insert the original file into its placeholder, by manipulating the source and target xml as DOM documents. Very early
      // in the loading process, the original file may not be specified yet (m_originalFile is empty)
      QDomDocument domInputFile;
      loadInputFileForErrorReport (domInputFile);
      QDomDocumentFragment fragmentFileFrom = domErrorReport.createDocumentFragment();
      if (!domInputFile.isNull()) {
        fragmentFileFrom.appendChild (domErrorReport.importNode (domInputFile.documentElement(), DEEP_COPY));
      }
      QDomNodeList nodesFileTo = domErrorReport.elementsByTagName (DOCUMENT_SERIALIZE_FILE);
      if (nodesFileTo.count () > 0) {
        QDomNode nodeFileTo = nodesFileTo.at (0);
        nodeFileTo.appendChild (fragmentFileFrom);
      }

      // Replace DOCUMENT_SERIALIZE_IMAGE by same node with CDATA removed, since:
      // 1) it is very big and working with smaller files, especially in emails, is easier
      // 2) removing the image better preserves user's privacy
      // 3) having the actual image does not help that much when debugging
      QDomNodeList nodesDocument = domErrorReport.elementsByTagName (DOCUMENT_SERIALIZE_DOCUMENT);
      for (int i = 0 ; i < nodesDocument.count(); i++) {
        QDomNode nodeDocument = nodesDocument.at (i);
        QDomElement elemImage = nodeDocument.firstChildElement(DOCUMENT_SERIALIZE_IMAGE);
        if (!elemImage.isNull()) {

          // Get old image attributes so we can create an empty document with the same size
          if (elemImage.hasAttribute (DOCUMENT_SERIALIZE_IMAGE_WIDTH) &&
              elemImage.hasAttribute (DOCUMENT_SERIALIZE_IMAGE_HEIGHT)) {

            int width = elemImage.attribute(DOCUMENT_SERIALIZE_IMAGE_WIDTH).toInt();
            int height = elemImage.attribute(DOCUMENT_SERIALIZE_IMAGE_HEIGHT).toInt();

            QDomNode nodeReplacement;
            QDomElement elemReplacement = nodeReplacement.toElement();
            elemReplacement.setAttribute (DOCUMENT_SERIALIZE_IMAGE_WIDTH, width);
            elemReplacement.setAttribute (DOCUMENT_SERIALIZE_IMAGE_HEIGHT, height);

            // Replace with the new and then remove the old
            nodeDocument.insertBefore (nodeReplacement,
                                       elemImage);
            nodeDocument.removeChild(elemImage);
          }
        }
      }
    }

    DlgErrorReport dlg (domErrorReport.toString());
    dlg.exec();
  }
}

GraphicsScene &MainWindow::scene ()
{
  ENGAUGE_CHECK_PTR (m_scene);
  return *m_scene;
}

BackgroundImage MainWindow::selectOriginal(BackgroundImage backgroundImage)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::selectBackgroundOriginal";

  BackgroundImage previousBackground = (BackgroundImage) m_cmbBackground->currentData().toInt();

  int index = m_cmbBackground->findData (backgroundImage);
  ENGAUGE_ASSERT (index >= 0);

  m_cmbBackground->setCurrentIndex(index);

  return previousBackground;
}

QString MainWindow::selectedGraphCurve () const
{
  return m_cmbCurve->currentText ();
}

void MainWindow::setCurrentFile (const QString &fileName)
{
  const QString PLACEHOLDER ("[*]");

  QString title = QString (tr ("Engauge Digitizer %1")
                           .arg (VERSION_NUMBER));

  QString fileNameStripped = fileName;
  if (!fileName.isEmpty()) {

    // Strip out path and file extension
    QFileInfo fileInfo (fileName);
    fileNameStripped = fileInfo.baseName();

    title += QString (": %1")
             .arg (fileNameStripped);
  }

  m_currentFile = fileNameStripped;

  // To prevent "QWidget::setWindowModified: The window title does not contain a [*] placeholder" warnings,
  // we always append a placeholder
  title += PLACEHOLDER;

  setWindowTitle (title);
}

void MainWindow::setCurrentPathFromFile (const QString &fileName)
{
  QDir dir = QFileInfo (fileName).absoluteDir();

  if (dir.exists ()) {

    bool success = QDir::setCurrent (dir.absolutePath ()); // Return to chosen directory the next time
    ENGAUGE_ASSERT (success);

  } else {

    // File was a url so it is irrelevant to the current directory
  }
}

void MainWindow::setPixmap (const QPixmap &pixmap)
{
  m_digitizeStateContext->setImageIsLoaded (true);

  updateImages (pixmap);
  updateViewedBackground ();
}

void MainWindow::settingsRead ()
{
  QSettings settings (SETTINGS_ENGAUGE, SETTINGS_DIGITIZER);

  settingsReadEnvironment (settings);
  settingsReadMainWindow (settings);
}

void MainWindow::settingsReadEnvironment (QSettings &settings)
{
  settings.beginGroup (SETTINGS_GROUP_ENVIRONMENT);
  QDir::setCurrent (settings.value (SETTINGS_CURRENT_DIRECTORY,
                                    QDir::currentPath ()).toString ());
  settings.endGroup ();
}

void MainWindow::settingsReadMainWindow (QSettings &settings)
{
  settings.beginGroup(SETTINGS_GROUP_MAIN_WINDOW);

  // Window geometry
  resize (settings.value (SETTINGS_SIZE,
                          QSize (400, 400)).toSize ());
  move (settings.value (SETTINGS_POS,
                        QPoint (200, 200)).toPoint ());

  // Background toolbar visibility
  bool viewBackgroundToolBar = settings.value (SETTINGS_VIEW_BACKGROUND_TOOLBAR,
                                               true).toBool ();
  m_actionViewBackground->setChecked (viewBackgroundToolBar);
  m_toolBackground->setVisible (viewBackgroundToolBar);
  BackgroundImage backgroundImage = (BackgroundImage) settings.value (SETTINGS_BACKGROUND_IMAGE,
                                                                      BACKGROUND_IMAGE_FILTERED).toInt ();
  int indexBackground = m_cmbBackground->findData (QVariant (backgroundImage));
  m_cmbBackground->setCurrentIndex (indexBackground);

  // Digitize toolbar visibility
  bool viewDigitizeToolBar = settings.value (SETTINGS_VIEW_DIGITIZE_TOOLBAR,
                                             true).toBool ();
  m_actionViewDigitize->setChecked (viewDigitizeToolBar);
  m_toolDigitize->setVisible (viewDigitizeToolBar);

  // Views toolbar visibility
  bool viewSettingsViewsToolBar = settings.value (SETTINGS_VIEW_SETTINGS_VIEWS_TOOLBAR,
                                                  true).toBool ();
  m_actionViewSettingsViews->setChecked (viewSettingsViewsToolBar);
  m_toolSettingsViews->setVisible (viewSettingsViewsToolBar);

  // Tooltips visibility
  bool viewToolTips = settings.value (SETTINGS_VIEW_TOOL_TIPS,
                                      true).toBool ();
  m_actionViewToolTips->setChecked (viewToolTips);
  loadToolTips ();

  // Statusbar visibility
  StatusBarMode statusBarMode = (StatusBarMode) settings.value (SETTINGS_VIEW_STATUS_BAR,
                                                                false).toInt ();
  m_statusBar->setStatusBarMode (statusBarMode);
  m_actionStatusNever->setChecked (statusBarMode == STATUS_BAR_MODE_NEVER);
  m_actionStatusTemporary->setChecked (statusBarMode == STATUS_BAR_MODE_TEMPORARY);
  m_actionStatusAlways->setChecked (statusBarMode == STATUS_BAR_MODE_ALWAYS);

  settings.endGroup();
}

void MainWindow::settingsWrite ()
{
  QSettings settings (SETTINGS_ENGAUGE, SETTINGS_DIGITIZER);

  settings.beginGroup (SETTINGS_GROUP_ENVIRONMENT);
  settings.setValue (SETTINGS_CURRENT_DIRECTORY, QDir::currentPath ());
  settings.endGroup ();

  settings.beginGroup (SETTINGS_GROUP_MAIN_WINDOW);
  settings.setValue (SETTINGS_SIZE, size ());
  settings.setValue (SETTINGS_POS, pos ());
  settings.setValue (SETTINGS_VIEW_BACKGROUND_TOOLBAR, m_actionViewBackground->isChecked());
  settings.setValue (SETTINGS_BACKGROUND_IMAGE, m_cmbBackground->currentData().toInt());
  settings.setValue (SETTINGS_VIEW_DIGITIZE_TOOLBAR, m_actionViewDigitize->isChecked ());
  settings.setValue (SETTINGS_VIEW_STATUS_BAR, m_statusBar->statusBarMode ());
  settings.setValue (SETTINGS_VIEW_SETTINGS_VIEWS_TOOLBAR, m_actionViewSettingsViews->isChecked ());
  settings.setValue (SETTINGS_VIEW_TOOL_TIPS, m_actionViewToolTips->isChecked ());
  settings.endGroup ();
}

void MainWindow::setupAfterLoad (const QString &fileName,
                                 const QString &temporaryMessage)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::setupAfterLoad"
                              << " file=" << fileName.toLatin1().data()
                              << " message=" << temporaryMessage.toLatin1().data();

  // Next line assumes CmdMediator for the NEW Document is already stored in m_cmdMediator
  m_digitizeStateContext->bindToCmdMediator (m_cmdMediator);

  connect (m_actionEditUndo, SIGNAL (triggered ()), m_cmdMediator, SLOT (undo ()));
  connect (m_actionEditUndo, SIGNAL (triggered ()), m_cmdStackShadow, SLOT (slotUndo ()));
  connect (m_actionEditRedo, SIGNAL (triggered ()), m_cmdMediator, SLOT (redo ())); // No effect until CmdMediator::undo and CmdStackShadow::slotUndo get called
  connect (m_actionEditRedo, SIGNAL (triggered ()), m_cmdStackShadow, SLOT (slotRedo ())); // No effect after CmdMediator::undo and CmdStackShadow::slotUndo get called
  connect (m_cmdMediator, SIGNAL (canRedoChanged(bool)), this, SLOT (slotCanRedoChanged (bool)));
  connect (m_cmdMediator, SIGNAL (canUndoChanged(bool)), this, SLOT (slotCanUndoChanged (bool)));
  connect (m_cmdMediator, SIGNAL (redoTextChanged (const QString &)), this, SLOT (slotRedoTextChanged (const QString &)));
  connect (m_cmdMediator, SIGNAL (undoTextChanged (const QString &)), this, SLOT (slotUndoTextChanged (const QString &)));
  loadCurveListFromCmdMediator ();
  updateViewsOfSettings ();
  setPixmap (m_cmdMediator->pixmap ());
  slotViewZoomFill();

  setCurrentFile(fileName);
  m_statusBar->showTemporaryMessage (temporaryMessage);
  m_statusBar->wakeUp ();

  updateAfterCommand(); // Replace stale points by points in new Document
}

void MainWindow::slotCanRedoChanged (bool canRedo)
{
  LOG4CPP_DEBUG_S ((*mainCat)) << "MainWindow::slotCanRedoChanged";

  m_actionEditRedo->setEnabled (canRedo || m_cmdStackShadow->canRedo());
}

void MainWindow::slotCanUndoChanged (bool canUndo)
{
  LOG4CPP_DEBUG_S ((*mainCat)) << "MainWindow::slotCanUndoChanged";

  m_actionEditUndo->setEnabled (canUndo);
}

void MainWindow::slotCleanChanged(bool clean)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotCleanChanged";

  setWindowModified (!clean);
}

void MainWindow::slotCmbBackground(int currentIndex)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotCmbBackground";

  switch (currentIndex) {
    case BACKGROUND_IMAGE_NONE:
      if (!m_actionViewBackgroundNone->isChecked()) {
        m_actionViewBackgroundNone->toggle();
      }
      break;

    case BACKGROUND_IMAGE_ORIGINAL:
      if (!m_actionViewBackgroundOriginal->isChecked ()) {
          m_actionViewBackgroundOriginal->toggle();
      }
      break;

    case BACKGROUND_IMAGE_FILTERED:
      if (!m_actionViewBackgroundFiltered->isChecked ()) {
        m_actionViewBackgroundFiltered->toggle();
      }
      break;
  }

  updateViewedBackground();
}

void MainWindow::slotCmbCurve(int /* currentIndex */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotCmbCurve";

  updateViewedPoints();
  updateViewsOfSettings();
}

void MainWindow::slotContextMenuEvent (QString pointIdentifier)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotContextMenuEvent point=" << pointIdentifier.toLatin1 ().data ();

  m_digitizeStateContext->handleContextMenuEvent (pointIdentifier);
}

void MainWindow::slotDigitizeAxis ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotDigitizeAxis";

  m_digitizeStateContext->requestImmediateStateTransition (DIGITIZE_STATE_AXIS);
  m_cmbCurve->setEnabled (false);
}

void MainWindow::slotDigitizeColorPicker ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotDigitizeColorPicker";

  m_digitizeStateContext->requestImmediateStateTransition (DIGITIZE_STATE_COLOR_PICKER);
  m_cmbCurve->setEnabled (true);
}

void MainWindow::slotDigitizeCurve ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotDigitizeCurve";

  m_digitizeStateContext->requestImmediateStateTransition (DIGITIZE_STATE_CURVE);
  m_cmbCurve->setEnabled (true);
}

void MainWindow::slotDigitizePointMatch ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotDigitizePointMatch";

  m_digitizeStateContext->requestImmediateStateTransition (DIGITIZE_STATE_POINT_MATCH);
  m_cmbCurve->setEnabled (true);
}

void MainWindow::slotDigitizeSegment ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotDigitizeSegment";

  m_digitizeStateContext->requestImmediateStateTransition (DIGITIZE_STATE_SEGMENT);
  m_cmbCurve->setEnabled (true);
}

void MainWindow::slotDigitizeSelect ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotDigitizeSelect";

  m_digitizeStateContext->requestImmediateStateTransition (DIGITIZE_STATE_SELECT);
  m_cmbCurve->setEnabled (false);
}

void MainWindow::slotEditCopy ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotEditCopy";

  CmdCopy *cmd = new CmdCopy (*this,
                              m_cmdMediator->document(),
                              m_scene->selectedPointIdentifiers ());
  m_digitizeStateContext->appendNewCmd (cmd);
}

void MainWindow::slotEditCut ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotEditCut";

  CmdCut *cmd = new CmdCut (*this,
                            m_cmdMediator->document(),
                            m_scene->selectedPointIdentifiers ());
  m_digitizeStateContext->appendNewCmd (cmd);
}

void MainWindow::slotEditDelete ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotEditDelete";

  CmdDelete *cmd = new CmdDelete (*this,
                                  m_cmdMediator->document(),
                                  m_scene->selectedPointIdentifiers ());
  m_digitizeStateContext->appendNewCmd (cmd);
}

void MainWindow::slotEditPaste ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotEditPaste";
}

void MainWindow::slotFileExport ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFileExport";

  const int SELECTED_FILTER = 0;
  QString filter = QString ("Text CSV (*.%1);;Text TSV (*.%2);;All files (*.*)")
                   .arg (CSV_FILENAME_EXTENSION)
                   .arg (TSV_FILENAME_EXTENSION);
  QString defaultFileName = QString ("%1/%2.%3")
                            .arg (QDir::currentPath ())
                            .arg (m_currentFile)
                            .arg (CSV_FILENAME_EXTENSION);
  QString fileName = QFileDialog::getSaveFileName (this,
                                                   tr("Export"),
                                                   defaultFileName,
                                                   filter,
                                                   SELECTED_FILTER);
  if (!fileName.isEmpty ()) {

    QFile file (fileName);
    if (file.open(QIODevice::WriteOnly)) {

      QTextStream str (&file);

      ExportToFile exportStrategy;
      exportStrategy.exportToFile (cmdMediator().document().modelExport(),
                                   cmdMediator().document(),
                                   str);
    } else {

      QMessageBox::critical (0,
                             engaugeWindowTitle(),
                             tr ("Unable to export to file ") + fileName);
    }
  }
}

void MainWindow::slotFileImport ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFileImport";

  if (maybeSave ()) {

    QString filter;
    QTextStream str (&filter);

    // Compile a list of supported formats into a filter
    str << "Image Files (";
    QList<QByteArray> supportedImageFormats = QImageReader::supportedImageFormats();
    QList<QByteArray>::iterator itr;
    QString delimiter;
    for (itr = supportedImageFormats.begin (); itr != supportedImageFormats.end (); itr++) {
      QByteArray arr = *itr;
      str << delimiter << "*." << arr.data ();
      delimiter = " ";
    }
    str << ")";

    // Allow selection of files with strange suffixes in case the file extension was changed. Since
    // the default is the first filter, we add this afterwards (it is the off-nominal case)
    str << ";; All Files (*.*)";

    QString fileName = QFileDialog::getOpenFileName (this,
                                                     tr("Import Image"),
                                                     QDir::currentPath (),
                                                     filter);
    if (!fileName.isEmpty ()) {

      fileImport (fileName);

    }
  }
}

void MainWindow::slotFileImportDraggedImage(QImage image)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFileImportDraggedImage";

  loadImage ("",
             image);
}

void MainWindow::slotFileImportDraggedImageUrl(QUrl url)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFileImportDraggedImageUrl url=" << url.toString ().toLatin1 ().data ();

  m_loadImageFromUrl->startLoadImage (url);
}

void MainWindow::slotFileImportImage(QString fileName, QImage image)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFileImportImage fileName=" << fileName.toLatin1 ().data ();

  loadImage (fileName,
             image);
}

void MainWindow::slotFileOpen()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFileOpen";

  if (maybeSave ()) {

    // Allow selection of files with strange suffixes in case the file extension was changed. Since
    // the default is the first filter, the wildcard filter is added afterwards (it is the off-nominal case)
    QString filter = QString ("%1 (*.%2);; All Files (*.*)")
                     .arg (ENGAUGE_FILENAME_DESCRIPTION)
                     .arg (ENGAUGE_FILENAME_EXTENSION);

    QString fileName = QFileDialog::getOpenFileName (this,
                                                     tr("Open Document"),
                                                     QDir::currentPath (),
                                                     filter);
    if (!fileName.isEmpty ()) {

      loadDocumentFile (fileName);

    }
  }
}

void MainWindow::slotFilePrint()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFilePrint";

  QPrinter printer (QPrinter::HighResolution);
  QPrintDialog dlg (&printer, this);
  if (dlg.exec() == QDialog::Accepted) {
    QPainter painter (&printer);
    m_view->render (&painter);
    painter.end();
  }
}

bool MainWindow::slotFileSave()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFileSave";

  if (m_engaugeFile.isEmpty()) {
    return slotFileSaveAs();
  } else {
    return saveDocumentFile (m_engaugeFile);
  }
}

bool MainWindow::slotFileSaveAs()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotFileSaveAs";

  // Append engauge file extension if it is not already there
  QString filenameDefault = m_currentFile;
  if (!m_currentFile.endsWith (ENGAUGE_FILENAME_EXTENSION)) {
    filenameDefault = QString ("%1.%2")
                               .arg (m_currentFile)
                               .arg (ENGAUGE_FILENAME_EXTENSION);
  }

  if (!m_engaugeFile.isEmpty()) {
    filenameDefault = m_engaugeFile;
  }

  QString filterDigitizer = QString ("%1 (*.%2)")
                            .arg (ENGAUGE_FILENAME_DESCRIPTION)
                            .arg (ENGAUGE_FILENAME_EXTENSION);
  QString filterAll ("All files (*. *)");

  QStringList filters;
  filters << filterDigitizer;
  filters << filterAll;

  QFileDialog dlg(this);
  dlg.selectNameFilter (filterDigitizer);
  dlg.setNameFilters (filters);
  dlg.setWindowModality(Qt::WindowModal);
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.selectFile(filenameDefault);
  if (dlg.exec()) {

    QStringList files = dlg.selectedFiles();
    return saveDocumentFile(files.at(0));
  }

  return false;
}

void MainWindow::slotHelpAbout()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotHelpAbout";

  DlgAbout dlg (*this);
  dlg.exec ();
}

void MainWindow::slotKeyPress (Qt::Key key)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotKeyPress key=" << QKeySequence (key).toString().toLatin1 ().data ();

  m_digitizeStateContext->handleKeyPress (key);
}

void MainWindow::slotLeave ()
{
  LOG4CPP_DEBUG_S ((*mainCat)) << "MainWindow::slotLeave";

  m_digitizeStateContext->handleLeave ();
}

void MainWindow::slotMouseMove (QPointF pos)
{
//  LOG4CPP_DEBUG_S ((*mainCat)) << "MainWindow::slotMouseMove pos=" << QPointFToString (pos).toLatin1 ().data ();

  // Ignore mouse moves before Document is loaded
  if (m_cmdMediator != 0) {

    // Get status bar coordinates
    QString coordsScreen, coordsGraph, resolutionGraph;
    m_transformation.coordTextForStatusBar (pos,
                                            coordsScreen,
                                            coordsGraph,
                                            resolutionGraph);

    // Update status bar coordinates
    m_statusBar->setCoordinates (coordsScreen,
                                 coordsGraph,
                                 resolutionGraph);

    // Move lines
    m_scene->moveLinesWithDraggedPoints(m_cmdMediator->document().modelCurveStyles());
  }
}

void MainWindow::slotMousePress (QPointF pos)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotMousePress";

  m_view->resetPositionHasChangedFlags();

  m_digitizeStateContext->handleMousePress (pos);
}

void MainWindow::slotMouseRelease (QPointF pos)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotMouseRelease";

  m_digitizeStateContext->handleMouseRelease (pos);
}

void MainWindow::slotRecentFileAction ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotRecentFileAction";

  QAction *action = qobject_cast<QAction*>(sender ());

  if (action) {
    QString fileName = action->data().toString();
    loadDocumentFile (fileName);
  }
}

void MainWindow::slotRedoTextChanged (const QString &text)
{
  LOG4CPP_DEBUG_S ((*mainCat)) << "MainWindow::slotRedoTextChanged";

  QString completeText ("Redo");
  if (!text.isEmpty ()) {
    completeText += QString (" \"%1\"").arg (text);
  }
  m_actionEditRedo->setText (completeText);
}

void MainWindow::slotSetOverrideCursor (QCursor cursor)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSetOverrideCursor";

  m_digitizeStateContext->handleSetOverrideCursor (cursor);
}

void MainWindow::slotSettingsAxesChecker ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsAxesChecker";

  m_dlgSettingsAxesChecker->load (*m_cmdMediator);
  m_dlgSettingsAxesChecker->show ();
}

void MainWindow::slotSettingsColorFilter ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsColorFilter";

  m_dlgSettingsColorFilter->load (*m_cmdMediator);
  m_dlgSettingsColorFilter->show ();
}

void MainWindow::slotSettingsCoords ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsCoords";

  m_dlgSettingsCoords->load (*m_cmdMediator);
  m_dlgSettingsCoords->show ();
}

void MainWindow::slotSettingsCurveProperties ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsCoords";

  m_dlgSettingsCurveProperties->load (*m_cmdMediator);
  m_dlgSettingsCurveProperties->setCurveName (selectedGraphCurve ());
  m_dlgSettingsCurveProperties->show ();
}

void MainWindow::slotSettingsCurves ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsCoords";

  m_dlgSettingsCurves->load (*m_cmdMediator);
  m_dlgSettingsCurves->show ();
}

void MainWindow::slotSettingsExport ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsCoords";

  m_dlgSettingsExport->load (*m_cmdMediator);
  m_dlgSettingsExport->show ();
}

void MainWindow::slotSettingsGridRemoval ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsCoords";

  m_dlgSettingsGridRemoval->load (*m_cmdMediator);
  m_dlgSettingsGridRemoval->show ();
}

void MainWindow::slotSettingsPointMatch ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsCoords";

  m_dlgSettingsPointMatch->load (*m_cmdMediator);
  m_dlgSettingsPointMatch->show ();
}

void MainWindow::slotSettingsSegments ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotSettingsCoords";

  m_dlgSettingsSegments->load (*m_cmdMediator);
  m_dlgSettingsSegments->show ();
}

void MainWindow::slotUndoTextChanged (const QString &text)
{
  LOG4CPP_DEBUG_S ((*mainCat)) << "MainWindow::slotUndoTextChanged";

  QString completeText ("Undo");
  if (!text.isEmpty ()) {
    completeText += QString (" \"%1\"").arg (text);
  }
  m_actionEditUndo->setText (completeText);
}

void MainWindow::slotViewGroupBackground(QAction *action)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewGroupBackground";

  // Set the combobox
  int indexBackground;
  if (action == m_actionViewBackgroundNone) {
    indexBackground = m_cmbBackground->findData (QVariant (BACKGROUND_IMAGE_NONE));
  } else if (action == m_actionViewBackgroundOriginal) {
    indexBackground = m_cmbBackground->findData (QVariant (BACKGROUND_IMAGE_ORIGINAL));
  } else if (action == m_actionViewBackgroundFiltered) {
    indexBackground = m_cmbBackground->findData (QVariant (BACKGROUND_IMAGE_FILTERED));
  } else {
    ENGAUGE_ASSERT (false);
  }
  m_cmbBackground->setCurrentIndex (indexBackground);

  updateViewedBackground();
}

void MainWindow::slotViewGroupPoints(QAction * /* action */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewGroupPoints";

  updateViewedPoints ();
}

void MainWindow::slotViewGroupStatus(QAction *action)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewGroupStatus";

  ENGAUGE_CHECK_PTR (m_statusBar); // At startup, make sure status bar is already set up when View menu gets initialized

  if (action == m_actionStatusNever) {
    m_statusBar->setStatusBarMode(STATUS_BAR_MODE_NEVER);
  } else if (action == m_actionStatusTemporary) {
    m_statusBar->setStatusBarMode(STATUS_BAR_MODE_TEMPORARY);
  } else {
    m_statusBar->setStatusBarMode(STATUS_BAR_MODE_ALWAYS);
  }
}

void MainWindow::slotViewToolBarBackground ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewToolBarBackground";

  if (m_actionViewBackground->isChecked ()) {
    m_toolBackground->show();
  } else {
    m_toolBackground->hide();
  }
}

void MainWindow::slotViewToolBarDigitize ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewToolBarDigitize";

  if (m_actionViewDigitize->isChecked ()) {
    m_toolDigitize->show();
  } else {
    m_toolDigitize->hide();
  }
}

void MainWindow::slotViewToolBarSettingsViews ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewToolBarSettingsViews";

  if (m_actionViewSettingsViews->isChecked ()) {
    m_toolSettingsViews->show();
  } else {
    m_toolSettingsViews->hide();
  }
}

void MainWindow::slotViewToolTips ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewToolTips";

  loadToolTips();
}

void MainWindow::slotViewZoom(int zoom)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewZoom";

  // Update zoom controls and apply the zoom factor
  switch ((ZoomFactor) zoom) {
    case ZOOM_16_TO_1:
      m_actionZoom16To1->setChecked(true);
      slotViewZoom16To1 ();
      break;
    case ZOOM_8_TO_1:
      m_actionZoom8To1->setChecked(true);
      slotViewZoom8To1 ();
      break;
    case ZOOM_4_TO_1:
      m_actionZoom4To1->setChecked(true);
      slotViewZoom4To1 ();
      break;
    case ZOOM_2_TO_1:
      m_actionZoom2To1->setChecked(true);
      slotViewZoom2To1 ();
      break;
    case ZOOM_1_TO_1:
      m_actionZoom1To1->setChecked(true);
      slotViewZoom1To1 ();
      break;
    case ZOOM_1_TO_2:
      m_actionZoom1To2->setChecked(true);
      slotViewZoom1To2 ();
      break;
    case ZOOM_1_TO_4:
      m_actionZoom1To4->setChecked(true);
      slotViewZoom1To4 ();
      break;
    case ZOOM_1_TO_8:
      m_actionZoom1To8->setChecked(true);
      slotViewZoom1To8 ();
      break;
    case ZOOM_1_TO_16:
      m_actionZoom1To16->setChecked(true);
      slotViewZoom1To16 ();
      break;
    case ZOOM_FILL:
      m_actionZoomFill->setChecked(true);
      slotViewZoomFill ();
      break;
  }
}

void MainWindow::slotViewZoom16To1 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewZoom16To1";

  QTransform transform;
  transform.scale (16.0, 16.0);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_16_TO_1);
}

void MainWindow::slotViewZoom8To1 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewZoom8To1";

  QTransform transform;
  transform.scale (8.0, 8.0);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_8_TO_1);
}

void MainWindow::slotViewZoom4To1 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewZoom4To1";

  QTransform transform;
  transform.scale (4.0, 4.0);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_4_TO_1);
}

void MainWindow::slotViewZoom2To1 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotZoom2To1";

  QTransform transform;
  transform.scale (2.0, 2.0);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_2_TO_1);
}

void MainWindow::slotViewZoom1To1 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewZoom1To1";

  QTransform transform;
  transform.scale (1.0, 1.0);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_1_TO_1);
}

void MainWindow::slotViewZoom1To2 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotZoom1To2";

  QTransform transform;
  transform.scale (0.5, 0.5);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_1_TO_2);
}

void MainWindow::slotViewZoom1To4 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotZoom1To4";

  QTransform transform;
  transform.scale (0.25, 0.25);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_1_TO_4);
}

void MainWindow::slotViewZoom1To8 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotZoom1To8";

  QTransform transform;
  transform.scale (0.125, 0.125);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_1_TO_8);
}

void MainWindow::slotViewZoom1To16 ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotZoom1To16";

  QTransform transform;
  transform.scale (0.0625, 0.0625);
  m_view->setTransform (transform);
  emit signalZoom(ZOOM_1_TO_16);
}

void MainWindow::slotViewZoomFill ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewZoomFill";

  m_view->fitInView (m_imageUnfiltered);
  emit signalZoom(ZOOM_FILL);
}

void MainWindow::slotViewZoomIn ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewZoomIn";

  // Try to zoom in. First determine what the next zoom factor should be

  bool goto16To1 = false, goto8To1 = false, goto4To1 = false, goto2To1 = false;
  bool goto1To1 = false;
  bool goto1To2 = false, goto1To4 = false, goto1To8 = false, goto1To16 = false;
  if (m_actionZoomFill->isChecked ()) {

    // Zooming in means user probably wants the more squished direction to be zoomed in by one step
    double xScale = m_view->transform().m11();
    double yScale = m_view->transform().m22();
    double scale = qMin(xScale, yScale);
    if (scale < 0.125) {
      goto1To8 = true;
    } else if (scale < 0.25) {
      goto1To4 = true;
    } else if (scale < 0.5) {
      goto1To2 = true;
    } else if (scale < 1) {
      goto1To1 = true;
    } else if (scale < 2) {
      goto2To1 = true;
    } else if (scale < 4) {
      goto4To1 = true;
    } else if (scale < 8) {
      goto8To1 = true;
    } else {
      goto1To16 = true;
    }
  } else {
    goto16To1 = m_actionZoom8To1->isChecked ();
    goto8To1 = m_actionZoom4To1->isChecked ();
    goto4To1 = m_actionZoom2To1->isChecked ();
    goto2To1 = m_actionZoom1To1->isChecked ();
    goto1To1 = m_actionZoom1To2->isChecked ();
    goto1To2 = m_actionZoom1To4->isChecked ();
    goto1To4 = m_actionZoom1To8->isChecked ();
    goto1To8 = m_actionZoom1To16->isChecked ();
  }

  // Update controls and apply zoom factor
  if (goto16To1) {
    m_actionZoom16To1->setChecked (true);
    slotViewZoom16To1 ();
  } else if (goto8To1) {
    m_actionZoom8To1->setChecked (true);
    slotViewZoom8To1 ();
  } else if (goto4To1) {
    m_actionZoom4To1->setChecked (true);
    slotViewZoom4To1 ();
  } else if (goto2To1) {
    m_actionZoom2To1->setChecked (true);
    slotViewZoom2To1 ();
  } else if (goto1To1) {
    m_actionZoom1To1->setChecked (true);
    slotViewZoom1To1 ();
  } else if (goto1To2) {
    m_actionZoom1To2->setChecked (true);
    slotViewZoom1To2 ();
  } else if (goto1To4) {
    m_actionZoom1To4->setChecked (true);
    slotViewZoom1To4 ();
  } else if (goto1To8) {
    m_actionZoom1To8->setChecked (true);
    slotViewZoom1To8 ();
  } else if (goto1To16) {
    m_actionZoom1To16->setChecked (true);
    slotViewZoom1To16 ();
  }
}

void MainWindow::slotViewZoomOut ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::slotViewZoomOut";

  // Try to zoom out. First determine what the next zoom factor should be

  bool goto16To1 = false, goto8To1 = false, goto4To1 = false, goto2To1 = false;
  bool goto1To1 = false;
  bool goto1To2 = false, goto1To4 = false, goto1To8 = false, goto1To16 = false;
  if (m_actionZoomFill->isChecked ()) {

    // Zooming out means user probably wants the less squished direction to be zoomed out by one step
    double xScale = m_view->transform().m11();
    double yScale = m_view->transform().m22();
    double scale = qMax(xScale, yScale);
    if (scale > 8) {
      goto8To1 = true;
    } else if (scale > 4) {
      goto4To1 = true;
    } else if (scale > 2) {
      goto2To1 = true;
    } else if (scale > 1) {
      goto1To1 = true;
    } else if (scale > 0.5) {
      goto1To2 = true;
    } else if (scale > 0.25) {
      goto1To4 = true;
    } else if (scale > 0.125) {
      goto1To8 = true;
    } else {
      goto1To16 = true;
    }
  } else {
    goto8To1 = m_actionZoom16To1->isChecked ();
    goto4To1 = m_actionZoom8To1->isChecked ();
    goto2To1 = m_actionZoom4To1->isChecked ();
    goto1To1 = m_actionZoom2To1->isChecked ();
    goto1To2 = m_actionZoom1To1->isChecked ();
    goto1To4 = m_actionZoom1To2->isChecked ();
    goto1To8 = m_actionZoom1To4->isChecked ();
    goto1To16 = m_actionZoom1To8->isChecked ();
  }

  // Update controls and apply zoom factor
  if (goto1To16) {
    m_actionZoom1To16->setChecked (true);
    slotViewZoom1To16 ();
  } else if (goto1To8) {
    m_actionZoom1To8->setChecked (true);
    slotViewZoom1To8 ();
  } else if (goto1To4) {
    m_actionZoom1To4->setChecked (true);
    slotViewZoom1To4 ();
  } else if (goto1To2) {
    m_actionZoom1To2->setChecked (true);
    slotViewZoom1To2 ();
  } else if (goto1To1) {
    m_actionZoom1To1->setChecked (true);
    slotViewZoom1To1 ();
  } else if (goto2To1) {
    m_actionZoom2To1->setChecked (true);
    slotViewZoom2To1 ();
  } else if (goto4To1) {
    m_actionZoom4To1->setChecked (true);
    slotViewZoom4To1 ();
  } else if (goto8To1) {
    m_actionZoom8To1->setChecked (true);
    slotViewZoom8To1 ();
  } else if (goto16To1) {
    m_actionZoom16To1->setChecked (true);
    slotViewZoom16To1 ();
  }
}

bool MainWindow::transformIsDefined() const
{
  return m_transformation.transformIsDefined();
}

void MainWindow::updateAfterCommand ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateAfterCommand";

  ENGAUGE_CHECK_PTR (m_cmdMediator);

  // Update transformation stuff, including the graph coordinates of every point in the Document, so coordinates in
  // status bar are up to date. Point coordinates in Document are also updated
  updateAfterCommandStatusBarCoords ();

  // Update the QGraphicsScene with the populated Curves. This requires the points in the Document to be already updated
  // by updateAfterCommandStatusBarCoords
  m_scene->updateAfterCommand (*m_cmdMediator);

  updateControls ();
}

void MainWindow::updateAfterCommandStatusBarCoords ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateAfterCommandStatusBarCoords";

  // For some reason, mapFromGlobal(QCursor::pos) differs from event->pos by a little bit. We must compensate for
  // this so cursor coordinates in status bar match the DlgEditPoint inputs initially. After the mouse moves
  // the problem disappears since event->pos is available and QCursor::pos is no longer needed
  const QPoint HACK_SO_GRAPH_COORDINATE_MATCHES_INPUT (1, 1);

  Transformation m_transformationBefore (m_transformation);

  m_transformation.update (!m_currentFile.isEmpty (), *m_cmdMediator);

  // Trigger state transitions for transformation if appropriate
  if (!m_transformationBefore.transformIsDefined() && m_transformation.transformIsDefined()) {

    // Transition from undefined to defined
    m_transformationStateContext->triggerStateTransition(TRANSFORMATION_STATE_DEFINED,
                                                         cmdMediator(),
                                                         m_transformation);

  } else if (m_transformationBefore.transformIsDefined() && !m_transformation.transformIsDefined()) {

    // Transition from defined to undefined
    m_transformationStateContext->triggerStateTransition(TRANSFORMATION_STATE_UNDEFINED,
                                                         cmdMediator(),
                                                         m_transformation);

  } else if (m_transformation.transformIsDefined() && (m_transformationBefore != m_transformation)) {

    // There was not a define/undefined or undefined/defined transition, but the transformation changed so we
    // need to update the Checker
    m_transformationStateContext->updateAxesChecker(cmdMediator(),
                                                    m_transformation);

  }

  if (m_transformation.transformIsDefined()) {

    // Update transformation. This is done regardless of whether or not the transformation was defined before
    m_cmdMediator->applyTransformation (m_transformation);

  }

  QPoint posLocal = m_view->mapFromGlobal (QCursor::pos ()) - HACK_SO_GRAPH_COORDINATE_MATCHES_INPUT;
  QPointF posScreen = m_view->mapToScene (posLocal);

  slotMouseMove (posScreen); // Update the status bar coordinates to reflect the newly updated transformation
}

void MainWindow::updateAfterMouseRelease ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateAfterMouseRelease";

  updateControls ();
}

void MainWindow::updateControls ()
{
//  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateControls";

  m_cmbBackground->setEnabled (!m_currentFile.isEmpty ());

  m_menuFileOpenRecent->setEnabled ((m_actionRecentFiles.count () > 0) &&
                                    (m_actionRecentFiles.at(0)->isVisible ())); // Need at least one visible recent file entry
  m_actionSave->setEnabled (!m_engaugeFile.isEmpty ());
  m_actionSaveAs->setEnabled (!m_currentFile.isEmpty ());
  m_actionExport->setEnabled (!m_currentFile.isEmpty ());
  m_actionPrint->setEnabled (!m_currentFile.isEmpty ());

  if (m_cmdMediator == 0) {
    m_actionEditUndo->setEnabled (false);
    m_actionEditRedo->setEnabled (false);
  } else {
    m_actionEditUndo->setEnabled (m_cmdMediator->canUndo ());
    m_actionEditRedo->setEnabled (m_cmdMediator->canRedo () || m_cmdStackShadow->canRedo ());
  }
  m_actionEditCut->setEnabled (m_scene->selectedItems().count () > 0);
  m_actionEditCopy->setEnabled (m_scene->selectedItems().count () > 0);
  m_actionEditPaste->setEnabled (false);
  m_actionEditDelete->setEnabled (m_scene->selectedItems().count () > 0);

  m_actionDigitizeAxis->setEnabled (!m_currentFile.isEmpty ());
  m_actionDigitizeCurve ->setEnabled (!m_currentFile.isEmpty ());
  m_actionDigitizePointMatch->setEnabled (!m_currentFile.isEmpty ());
  m_actionDigitizeColorPicker->setEnabled (!m_currentFile.isEmpty ());
  m_actionDigitizeSegment->setEnabled (!m_currentFile.isEmpty ());
  m_actionDigitizeSelect->setEnabled (!m_currentFile.isEmpty ());

  m_actionViewBackground->setEnabled (!m_currentFile.isEmpty());
  m_actionViewDigitize->setEnabled (!m_currentFile.isEmpty ());
  m_actionViewSettingsViews->setEnabled (!m_currentFile.isEmpty ());

  m_actionSettingsCoords->setEnabled (!m_currentFile.isEmpty ());
  m_actionSettingsCurveProperties->setEnabled (!m_currentFile.isEmpty ());
  m_actionSettingsCurves->setEnabled (!m_currentFile.isEmpty ());
  m_actionSettingsExport->setEnabled (!m_currentFile.isEmpty ());
  m_actionSettingsColorFilter->setEnabled (!m_currentFile.isEmpty ());
  m_actionSettingsAxesChecker->setEnabled (!m_currentFile.isEmpty ());
  m_actionSettingsGridRemoval->setEnabled (!m_currentFile.isEmpty ());
  m_actionSettingsPointMatch->setEnabled (!m_currentFile.isEmpty ());
  m_actionSettingsSegments->setEnabled (!m_currentFile.isEmpty ());

  m_groupBackground->setEnabled (!m_currentFile.isEmpty ());
  m_groupPoints->setEnabled (!m_currentFile.isEmpty ());
  m_groupZoom->setEnabled (!m_currentFile.isEmpty ());

  m_actionZoomIn->setEnabled (!m_currentFile.isEmpty ()); // Disable at startup so shortcut has no effect
  m_actionZoomOut->setEnabled (!m_currentFile.isEmpty ()); // Disable at startup so shortcut has no effect
}

void MainWindow::updateImages (const QPixmap &pixmap)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateImages";

  removePixmaps ();

  // Empty background
  QPixmap pixmapNone (pixmap);
  pixmapNone.fill (Qt::white);
  m_imageNone = m_scene->addPixmap (pixmapNone);
  m_imageNone->setData (DATA_KEY_IDENTIFIER, "view");
  m_imageNone->setData (DATA_KEY_GRAPHICS_ITEM_TYPE, GRAPHICS_ITEM_TYPE_IMAGE);

  // Unfiltered original image
  m_imageUnfiltered = m_scene->addPixmap (pixmap);
  m_imageUnfiltered->setData (DATA_KEY_IDENTIFIER, "view");
  m_imageUnfiltered->setData (DATA_KEY_GRAPHICS_ITEM_TYPE, GRAPHICS_ITEM_TYPE_IMAGE);

  // Reset scene rectangle or else small image after large image will be off-center
  m_scene->setSceneRect (m_imageUnfiltered->boundingRect ());

  // Filtered image
  ColorFilter filter;
  QImage imageUnfiltered (pixmap.toImage ());
  QImage imageFiltered (pixmap.width (),
                        pixmap.height (),
                        QImage::Format_RGB32);
  QRgb rgbBackground = filter.marginColor (&imageUnfiltered);
  filter.filterImage (imageUnfiltered,
                      imageFiltered,
                      cmdMediator().document().modelColorFilter().colorFilterMode(selectedGraphCurve ()),
                      cmdMediator().document().modelColorFilter().low(selectedGraphCurve ()),
                      cmdMediator().document().modelColorFilter().high(selectedGraphCurve ()),
                      rgbBackground);

  m_imageFiltered = m_scene->addPixmap (QPixmap::fromImage (imageFiltered));
  m_imageFiltered->setData (DATA_KEY_IDENTIFIER, "view");
  m_imageFiltered->setData (DATA_KEY_GRAPHICS_ITEM_TYPE, GRAPHICS_ITEM_TYPE_IMAGE);
}

void MainWindow::updateRecentFileList()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateRecentFileList";

  QSettings settings (SETTINGS_ENGAUGE, SETTINGS_DIGITIZER);
  QStringList recentFilePaths = settings.value(SETTINGS_RECENT_FILE_LIST).toStringList();

  // Determine the desired size of the path list
  unsigned int count = recentFilePaths.size();
  if (count > MAX_RECENT_FILE_LIST_SIZE) {
    count = MAX_RECENT_FILE_LIST_SIZE;
  }

  // Add visible entries
  unsigned int i;
  for (i = 0; i < count; i++) {
    QString strippedName = QFileInfo (recentFilePaths.at(i)).fileName();
    m_actionRecentFiles.at (i)->setText (strippedName);
    m_actionRecentFiles.at (i)->setData (recentFilePaths.at (i));
    m_actionRecentFiles.at (i)->setVisible (true);
  }

  // Hide any extra entries
  for (i = count; i < MAX_RECENT_FILE_LIST_SIZE; i++) {
    m_actionRecentFiles.at (i)->setVisible (false);
  }
}

void MainWindow::updateSettingsAxesChecker(const DocumentModelAxesChecker &modelAxesChecker)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsAxesChecker";

  m_cmdMediator->document().setModelAxesChecker(modelAxesChecker);
  m_transformationStateContext->updateAxesChecker (*m_cmdMediator,
                                                   m_transformation);
}

void MainWindow::updateSettingsColorFilter(const DocumentModelColorFilter &modelColorFilter)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsColorFilter";

  m_cmdMediator->document().setModelColorFilter(modelColorFilter);
  updateImages (cmdMediator().document().pixmap());
  updateViewedBackground ();
  updateViewsOfSettings();
}

void MainWindow::updateSettingsCoords(const DocumentModelCoords &modelCoords)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsCoords";

  m_cmdMediator->document().setModelCoords(modelCoords);
}

void MainWindow::updateSettingsCurves (const CurvesGraphs &curvesGraphs)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsCurves";

  m_cmdMediator->document().setCurvesGraphs (curvesGraphs);
  loadCurveListFromCmdMediator();
  updateViewsOfSettings();
}

void MainWindow::updateSettingsCurveStyles(const CurveStyles &modelCurveStyles)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsCurveStyles";

  m_scene->updateCurveStyles(modelCurveStyles);
  m_cmdMediator->document().setModelCurveStyles(modelCurveStyles);
  updateViewsOfSettings();
}

void MainWindow::updateSettingsExport(const DocumentModelExport &modelExport)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsExport";

  m_cmdMediator->document().setModelExport (modelExport);
}

void MainWindow::updateSettingsGridRemoval(const DocumentModelGridRemoval &modelGridRemoval)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsGridRemoval";

  m_cmdMediator->document().setModelGridRemoval(modelGridRemoval);
}

void MainWindow::updateSettingsPointMatch(const DocumentModelPointMatch &modelPointMatch)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsPointMatch";

  m_cmdMediator->document().setModelPointMatch(modelPointMatch);
}

void MainWindow::updateSettingsSegments(const DocumentModelSegments &modelSegments)
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateSettingsSegments";

  m_cmdMediator->document().setModelSegments(modelSegments);
}

void MainWindow::updateViewedBackground()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateViewedBackground";

  if (m_cmdMediator != 0) {

    BackgroundImage backgroundImage = (BackgroundImage) m_cmbBackground->currentData().toInt();

    m_imageNone->setVisible (backgroundImage == BACKGROUND_IMAGE_NONE);
    m_imageUnfiltered->setVisible (backgroundImage == BACKGROUND_IMAGE_ORIGINAL);
    m_imageFiltered->setVisible (backgroundImage == BACKGROUND_IMAGE_FILTERED);
  }
}

void MainWindow::updateViewedPoints ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "MainWindow::updateViewedPoints";

  if (m_actionViewPointsAll->isChecked ()) {

    m_scene->showPoints (true, true);

  } else if (m_actionViewPointsCurve->isChecked ()) {

    m_scene->showPoints (true, false, selectedGraphCurve ());

  } else if (m_actionViewPointsNone->isChecked ()) {

    m_scene->showPoints (false);

  } else {
    ENGAUGE_ASSERT (false);
  }
}

void MainWindow::updateViewsOfSettings ()
{
  QString activeCurve = m_digitizeStateContext->activeCurve ();

  updateViewsOfSettings (activeCurve);
}

void MainWindow::updateViewsOfSettings (const QString &activeCurve)
{
  if (activeCurve.isEmpty ()) {

    m_viewPointStyle->unsetPointStyle ();
    m_viewSegmentFilter->unsetColorFilterSettings ();


  } else {

    PointStyle pointStyle = m_cmdMediator->document().modelCurveStyles().curveStyle(activeCurve).pointStyle();
    m_viewPointStyle->setPointStyle (pointStyle);

    ColorFilterSettings colorFilterSettings = m_cmdMediator->document().modelColorFilter().colorFilterSettings(activeCurve);
    m_viewSegmentFilter->setColorFilterSettings (colorFilterSettings,
                                                 m_cmdMediator->pixmap ());

  }
}

GraphicsView &MainWindow::view ()
{
  ENGAUGE_CHECK_PTR (m_view);
  return *m_view;
}

const GraphicsView &MainWindow::view () const
{
  ENGAUGE_CHECK_PTR (m_view);
  return *m_view;
}
