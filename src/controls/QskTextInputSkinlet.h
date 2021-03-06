/******************************************************************************
 * QSkinny - Copyright (C) 2016 Uwe Rathmann
 * This file may be used under the terms of the QSkinny License, Version 1.0
 *****************************************************************************/

#ifndef QSK_TEXT_INPUT_SKINLET_H
#define QSK_TEXT_INPUT_SKINLET_H

#include "QskGlobal.h"
#include "QskSkinlet.h"

class QSK_EXPORT QskTextInputSkinlet : public QskSkinlet
{
    Q_GADGET

    using Inherited = QskSkinlet;

public:
    enum NodeRole
    {
        PanelRole
    };

    Q_INVOKABLE QskTextInputSkinlet( QskSkin* = nullptr );
    virtual ~QskTextInputSkinlet();

    virtual QRectF subControlRect( const QskSkinnable*,
        QskAspect::Subcontrol ) const override;

protected:
    virtual QSGNode* updateSubNode( const QskSkinnable*,
        quint8 nodeRole, QSGNode* ) const override;
};

#endif

