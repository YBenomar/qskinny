/******************************************************************************
 * QSkinny - Copyright (C) 2016 Uwe Rathmann
 * This file may be used under the terms of the QSkinny License, Version 1.0
 *****************************************************************************/

#include "QskSubWindowSkinlet.h"
#include "QskSubWindow.h"
#include "QskAspect.h"
#include "QskBoxBorderMetrics.h"

#include <QFontMetricsF>

QskSubWindowSkinlet::QskSubWindowSkinlet( QskSkin* skin ):
    Inherited( skin )
{
    appendNodeRoles( { PanelRole, TitleBarRole } );
}

QskSubWindowSkinlet::~QskSubWindowSkinlet() = default;

QRectF QskSubWindowSkinlet::subControlRect(
    const QskSkinnable* skinnable, QskAspect::Subcontrol subControl ) const
{
    const auto subWindow = static_cast< const QskSubWindow* >( skinnable );

    if ( subControl == QskSubWindow::TitleBar )
    {
        return titleBarRect( subWindow );
    }

    if ( subControl == QskSubWindow::Panel )
    {
        return subWindow->contentsRect();
    }

    return Inherited::subControlRect( skinnable, subControl );
}

QSGNode* QskSubWindowSkinlet::updateSubNode( const QskSkinnable* skinnable,
    quint8 nodeRole, QSGNode* node ) const
{
    const auto subWindow = static_cast< const QskSubWindow* >( skinnable );

    switch( nodeRole )
    {
        case PanelRole:
            return updateBoxNode( subWindow, node, QskSubWindow::Panel );

        case TitleBarRole:
            return updateBoxNode( subWindow, node, QskSubWindow::TitleBar );
    }

    return Inherited::updateSubNode( skinnable, nodeRole, node );
}

QRectF QskSubWindowSkinlet::titleBarRect( const QskSubWindow* subWindow ) const
{
    const auto border = subWindow->boxBorderMetricsHint( QskSubWindow::Panel );

    QRectF rect = subWindow->contentsRect().marginsRemoved( border.widths() );
    rect.setHeight( titleBarHeight( subWindow ) );

    return rect;
}

qreal QskSubWindowSkinlet::titleBarHeight( const QskSubWindow* subWindow ) const
{
    using namespace QskAspect;

    if ( !subWindow->isDecorated() )
        return 0;

    const QFontMetricsF fm( subWindow->effectiveFont( QskSubWindow::TitleBar ) );
    const QMarginsF margins = subWindow->marginsHint( QskSubWindow::TitleBar | Padding );

    const qreal height = fm.height() + margins.top() + margins.bottom();
    const qreal minHeight = subWindow->metric( QskSubWindow::TitleBar | MinimumHeight );

    return qMax( height, minHeight);
}

#include "moc_QskSubWindowSkinlet.cpp"
