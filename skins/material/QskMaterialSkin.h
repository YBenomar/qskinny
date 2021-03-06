/******************************************************************************
 * QSkinny - Copyright (C) 2016 Uwe Rathmann
 * This file may be used under the terms of the QSkinny License, Version 1.0
 *****************************************************************************/

#ifndef QSK_MATERIAL_SKIN_H
#define QSK_MATERIAL_SKIN_H

#include <QskSkin.h>
#include <memory>

class QSK_EXPORT QskMaterialSkin : public QskSkin
{
    Q_OBJECT

    using Inherited = QskSkin;

public:
    QskMaterialSkin( QObject* parent = nullptr );
    virtual ~QskMaterialSkin();

private:
    virtual void resetColors( const QColor& accent ) override;

    void initHints();

    void initCommonHints();

    void initDialogButtonBoxHints();
    void initDialogButtonHints();
    void initFocusIndicatorHints();
    void initInputPanelHints();
    void initListViewHints();
    void initPageIndicatorHints();
    void initPopupHints();
    void initPushButtonHints();
    void initScrollViewHints();
    void initSeparatorHints();
    void initSubWindowHints();
    void initSliderHints();
    void initTabButtonHints();
    void initTabBarHints();
    void initTabViewHints();
    void initTextInputHints();
    void initTextLabelHints();

    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

#endif
