#ifndef EXCOLORMODELSWICTHERS_H
#define EXCOLORMODELSWICTHERS_H

#include <QWidget>

#include "EXColorState.h"
#include "EXColorMixState.h"
#include "EXSettingsState.h"

class EXColorModelSwitchers : public QWidget
{
    Q_OBJECT

public:
    EXColorModelSwitchers(EXColorMixStateSP colorState, EXSettingsStateSP settingsState, QWidget *parent);
    ~EXColorModelSwitchers() override = default;
    void settingsChanged();

private:
    //EXColorStateSP m_colorState;
    EXColorMixStateSP m_colorMixState;
    EXSettingsStateSP m_settingsState;
};

#endif
