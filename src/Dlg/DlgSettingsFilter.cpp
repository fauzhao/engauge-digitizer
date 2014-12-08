#include "CmdMediator.h"
#include "CmdSettingsFilter.h"
#include "DlgSettingsFilter.h"
#include "Logger.h"
#include "MainWindow.h"
#include <QComboBox>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include "ViewPreview.h"
#include "ViewProfile.h"

const int PROFILE_HEIGHT_IN_ROWS = 6;

DlgSettingsFilter::DlgSettingsFilter(MainWindow &mainWindow) :
  DlgSettingsAbstractBase ("Filter", mainWindow),
  m_modelFilterBefore (0),
  m_modelFilterAfter (0)
{
  QWidget *subPanel = createSubPanel ();
  finishPanel (subPanel);
}

void DlgSettingsFilter::createControls (QGridLayout *layout, int &row)
{
  QLabel *labelProfile = new QLabel ("Filter parameter:");
  layout->addWidget (labelProfile, row++, 1);

  m_btnIntensity = new QRadioButton ("Intensity");
  m_btnIntensity->setWhatsThis (tr ("Filter the original image into black and white pixels using the Intensity parameter, "
                                    "to hide unimportant information and emphasize important information.\n\n"
                                    "The Intensity value of a pixel is computed from the red, green "
                                    "and blue components as I = (R + G + B) / 3"));
  connect (m_btnIntensity, SIGNAL (released ()), this, SLOT (slotIntensity ()));
  layout->addWidget (m_btnIntensity, row++, 1);

  m_btnForeground = new QRadioButton ("Foreground");
  m_btnForeground->setWhatsThis (tr ("Filter the original image into black and white pixels by isolating the foreground from the background, "
                                     "to hide unimportant information and emphasize important information.\n\n"
                                     "The background color is shown on the left side of the scale bar. All pixels with approximately "
                                     "the background color are considered part of the background, and all other pixels are considered "
                                     "part of the foreground"));
  connect (m_btnForeground, SIGNAL (released ()), this, SLOT (slotForeground ()));
  layout->addWidget (m_btnForeground, row++, 1);

  m_btnHue = new QRadioButton ("Hue");
  m_btnHue->setWhatsThis (tr ("Filter the original image into black and white pixels using the Hue component of the "
                              "Hue, Saturation and Value (HSV) color components, "
                              "to hide unimportant information and emphasize important information."));
  connect (m_btnHue, SIGNAL (released ()), this, SLOT (slotHue ()));
  layout->addWidget (m_btnHue, row++, 1);

  m_btnSaturation = new QRadioButton ("Saturation");
  m_btnSaturation->setWhatsThis (tr ("Filter the original image into black and white pixels using the Saturation component of the "
                                     "Hue, Saturation and Value (HSV) color components, "
                                     "to hide unimportant information and emphasize important information."));
  connect (m_btnSaturation, SIGNAL (released ()), this, SLOT (slotSaturation ()));
  layout->addWidget (m_btnSaturation, row++, 1);

  m_btnValue = new QRadioButton ("Value");
  m_btnValue->setWhatsThis (tr ("Filter the original image into black and white pixels using the Value component of the "
                                "Hue, Saturation and Value (HSV) color components, "
                                "to hide unimportant information and emphasize important information.\n\n"
                                "The Value component is also called the Lightness."));
  connect (m_btnValue, SIGNAL (released ()), this, SLOT (slotValue ()));
  layout->addWidget (m_btnValue, row++, 1);
}

void DlgSettingsFilter::createPreview (QGridLayout *layout, int &row)
{
  QLabel *labelPreview = new QLabel ("Preview");
  layout->addWidget (labelPreview, row++, 0, 1, 5);

  m_scenePreview = new QGraphicsScene (this);
  m_viewPreview = new ViewPreview (m_scenePreview, this);
  m_viewPreview->setWhatsThis (tr ("Preview window that shows how current settings affect the filtering of the original image."));
  m_viewPreview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_viewPreview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_viewPreview->setMinimumHeight (MINIMUM_PREVIEW_HEIGHT);

  layout->addWidget (m_viewPreview, row++, 0, 1, 5);
}

void DlgSettingsFilter::createProfileAndScale (QGridLayout *layout, int &row)
{
  QLabel *labelProfile = new QLabel ("Filter Parameter Histogram Profile");
  layout->addWidget (labelProfile, row++, 3);

  m_sceneProfile = new QGraphicsScene;
  m_viewProfile = new ViewProfile (m_sceneProfile);
  m_viewProfile->setWhatsThis (tr ("Histogram profile of the selected filter parameter. The two Dividers can be moved back and forth to adjust "
                                   "the range of filter parameter values that will be included in the filtered image. The clear portion will "
                                   "be included, and the shaded portion will be excluded."));

  layout->addWidget (m_viewProfile, row, 3, PROFILE_HEIGHT_IN_ROWS, 1);
  row += PROFILE_HEIGHT_IN_ROWS;

  m_scale = new QLabel;
  m_scale->setWhatsThis (tr ("This read-only box displays a graphical representation of the horizontal axis in the histogram profile above."));
  m_scale->setAutoFillBackground(true);
  m_scale->setPalette (QPalette (Qt::red));
  layout->addWidget (m_scale, row++, 3);
}

QWidget *DlgSettingsFilter::createSubPanel ()
{
  const int EMPTY_COLUMN_WIDTH = 40;

  QWidget *subPanel = new QWidget ();
  QGridLayout *layout = new QGridLayout (subPanel);
  subPanel->setLayout (layout);

  layout->setColumnStretch(0, 0); // Empty column
  layout->setColumnMinimumWidth(0, EMPTY_COLUMN_WIDTH);
  layout->setColumnStretch(1, 0); // Radio buttons
  layout->setColumnStretch(2, 0); // Empty column to put some space between previous and next columns, so they are not too close
  layout->setColumnMinimumWidth(2, 15);
  layout->setColumnStretch(3, 1); // Profile
  layout->setColumnMinimumWidth(4, EMPTY_COLUMN_WIDTH); // Empty column

  int rowLeft = 0, rowRight = 0;
  createControls (layout, rowLeft);
  createProfileAndScale (layout, rowRight);

  int row = qMax (rowLeft, rowRight);
  createPreview (layout, row);

  return subPanel;
}

void DlgSettingsFilter::handleOk ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "DlgSettingsFilter::handleOk";

  CmdSettingsFilter *cmd = new CmdSettingsFilter (mainWindow (),
                                                  cmdMediator ().document(),
                                                  *m_modelFilterBefore,
                                                  *m_modelFilterAfter);
  cmdMediator ().push (cmd);

  hide ();
}

void DlgSettingsFilter::load (CmdMediator &cmdMediator)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DlgSettingsFilter::load";

  setCmdMediator (cmdMediator);

  m_modelFilterBefore = new DocumentModelFilter (cmdMediator.document());
  m_modelFilterAfter = new DocumentModelFilter (cmdMediator.document());

  FilterParameter filterParameter = m_modelFilterAfter->filterParameter();
  m_btnIntensity->setChecked (filterParameter == FILTER_PARAMETER_INTENSITY);
  m_btnForeground->setChecked (filterParameter == FILTER_PARAMETER_FOREGROUND);
  m_btnHue->setChecked (filterParameter == FILTER_PARAMETER_HUE);
  m_btnSaturation->setChecked (filterParameter == FILTER_PARAMETER_SATURATION);
  m_btnValue->setChecked (filterParameter == FILTER_PARAMETER_VALUE);

  updateControls();
  enableOk (false); // Disable Ok button since there not yet any changes
  updatePreview();
}

void DlgSettingsFilter::slotForeground ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "DlgSettingsFilter::slotForeground";

  m_modelFilterAfter->setFilterParameter(FILTER_PARAMETER_FOREGROUND);
  updateControls();
  updatePreview();
}

void DlgSettingsFilter::slotHue ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "DlgSettingsFilter::slotHue";

  m_modelFilterAfter->setFilterParameter(FILTER_PARAMETER_HUE);
  updateControls();
  updatePreview();
}

void DlgSettingsFilter::slotIntensity ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "DlgSettingsFilter::slotIntensity";

  m_modelFilterAfter->setFilterParameter(FILTER_PARAMETER_INTENSITY);
  updateControls();
  updatePreview();
}

void DlgSettingsFilter::slotSaturation ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "DlgSettingsFilter::slotSaturation";

  m_modelFilterAfter->setFilterParameter(FILTER_PARAMETER_SATURATION);
  updateControls();
  updatePreview();
}

void DlgSettingsFilter::slotValue ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "DlgSettingsFilter::slotValue";

  m_modelFilterAfter->setFilterParameter(FILTER_PARAMETER_VALUE);
  updateControls();
  updatePreview();
}

void DlgSettingsFilter::updateControls ()
{

}

void DlgSettingsFilter::updatePreview ()
{

}