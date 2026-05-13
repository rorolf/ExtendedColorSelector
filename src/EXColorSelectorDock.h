#ifndef EXTENDEDCOLORSELECTORDOCK_H
#define EXTENDEDCOLORSELECTORDOCK_H

#include <QDockWidget>
#include <QObject>
#include <QPushButton>
#include <QVBoxLayout>

#include <KisPopupButton.h>
#include <kis_canvas2.h>
#include <kis_color_space_selector.h>
#include <kis_mainwindow_observer.h>

#include "EXChannelPlane.h"
#include "EXChannelSlider.h"
#include "EXColorModelSwitchers.h"
#include "EXColorPatchPopup.h"
#include "EXColorState.h"
#include "EXPortableColorSelector.h"
#include "EXSettingsDialog.h"
#include "EXSettingsState.h"


#include <QComboBox>
#include <QTabWidget>
#include "EXColorPatchWidget.h"
#include "EXMIDIPanelWidget.h"



//################################################################################
//## Forward Declarations
//################################################################################

// #include "EXActionbus.h"
class EXActionBus;


//################################################################################
//## Header subject
//################################################################################

class EXColorSelectorDock : public QDockWidget, public KisMainwindowObserver
{
    Q_OBJECT

public:
    EXColorSelectorDock();
    ~EXColorSelectorDock() override = default;

    void setViewManager(KisViewManager *kisview) override;
    void setCanvas(KoCanvasBase *canvas) override;
    void unsetCanvas() override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

    //TODO report preset name instead
    int selectedPreset() const;
    int selectedMixChannel() const;

private:
    friend class EXActionBus;

    KisCanvas2 *m_canvas;
    EXChannelPlane *m_plane;
    EXChannelSlidersGroup *m_sliders;
    EXColorModelSwitchers *m_colorModelSwitchers;
    EXGlobalSettingsDialog *m_globalSettings;
    EXPerColorModelSettingsDialog *m_settings;
    EXPortableColorSelector *m_portableSelector;
    EXColorPatchPopup *m_colorPatchPopup;

    EXColorMixStateSP m_colorMixState;
    EXSettingsStateSP m_settingsState;

    KisPopupButton *m_colorSpaceSelectorButton;
    KisColorSpaceSelector *m_colorSpaceSelector;
    QPushButton *m_useLayerColorSpaceButton;

    void updateSliders();

    KisPopupButton* m_colorSelectorPupupButton;

    QComboBox *m_presetSelector;
    QComboBox *m_colorModelSelector;
    int m_selectedColorPatchWidget=-1;
    std::array<EXColorPatchWidget*, 9> m_colorPatchWidgets;
    QButtonGroup *m_mixingModeSelector;
    EXColorPatchWidget *m_mixResultColorPatch;
    EXMIDIPanelWidget* m_midiPanel;
    QTabWidget* m_tabWidget;

    EXActionBus* m_actionBus;

    void loadColorsFromPreset(int newPreset);

Q_SIGNALS:
    void sigMixFromColorsButtonPressed();
    void sigMixFromGradientsButtonPressed();
    void sigColorPatchWidgetSelected(int clrPatchIndex);

public Q_SLOTS:
    void onColorSpaceSelected(const KoColorSpace *colorSpace);
    // TODO: switch to String-based setter
    void onNewPresetSelected(int activePreset);
};

#endif // EXTENDEDCOLORSELECTORDOCK_H
