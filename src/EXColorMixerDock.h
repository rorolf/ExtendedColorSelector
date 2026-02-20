

#ifndef EXTENDEDCOLORMIXERDOCK_H
#define EXTENDEDCOLORMIXERDOCK_H

#include <array>

#include <QDockWidget>
#include <QObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <QComboBox>
#include <QRadioButton>

#include <KisPopupButton.h>
#include <kis_canvas2.h>
#include <kis_color_space_selector.h>
#include <kis_mainwindow_observer.h>

#include "EXChannelPlane.h"
#include "EXChannelSlider.h"
#include "EXColorMixState.h"
#include "EXColorModelSwitchers.h"
#include "EXColorPatchPopup.h"
#include "EXColorPatchWidget.h"
#include "EXColorPresetStore.h"
#include "EXColorState.h"
#include "EXPortableColorSelector.h"
#include "EXSettingsDialog.h"
#include "EXSettingsState.h"

class EXColorMixerDock : public QDockWidget, public KisMainwindowObserver
{
    Q_OBJECT

public:
    EXColorMixerDock();
    ~EXColorMixerDock() override = default;

    void setViewManager(KisViewManager *kisview) override;
    void setCanvas(KoCanvasBase *canvas) override;
    void unsetCanvas() override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    KisCanvas2 *m_canvas;
    EXChannelPlane *m_plane;
    EXChannelSlidersGroup *m_sliders;

    QComboBox *m_presetSelector;
    QComboBox *m_colorSpaceSelector;

    //EXColorMixChannelSelector *m_colorMixChannelSelector;
    int m_selectedColorPatchWidget;
    std::array<EXColorPatchWidget*, 8> m_colorPatchWidgets;

    QButtonGroup *m_mixingModeSelector;

    EXColorMixStateSP m_colorMixState;

    EXColorModelSwitchers *m_colorModelSwitchers;
    EXGlobalSettingsDialog *m_globalSettings;
    EXPerColorModelSettingsDialog *m_settings;
    EXPortableColorSelector *m_portableSelector;
    EXColorPatchPopup *m_colorPatchPopup;

    EXColorStateSP m_colorState;
    EXColorPresetStoreSP m_colorPresets;
    EXSettingsStateSP m_settingsState;


    //KisPopupButton *m_colorSpaceSelectorButton;
    //KisColorSpaceSelector *m_colorSpaceSelector;
    // QPushButton *m_useLayerColorSpaceButton;

    void updateSliders();

Q_SIGNALS:
    void sigPresetSelected(const int preset);
    void sigColorMixChannelSelected(const int colorMixChannel);
    void sigColorMixChannelModeSelected(const bool isGradient);

public Q_SLOTS:
    void onColorSpaceSelected(const KoColorSpace *colorSpace);

};

#endif // EXTENDEDCOLORMIXERDOCK_H


