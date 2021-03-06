/******************************************************************************
 * QSkinny - Copyright (C) 2016 Uwe Rathmann
 * This file may be used under the terms of the QSkinny License, Version 1.0
 *****************************************************************************/

#include "QskInputContext.h"
#include "QskVirtualKeyboard.h"

#include "QskInputCompositionModel.h"
#include "QskPinyinCompositionModel.h"
#include "QskHunspellCompositionModel.h"

#include <QskDialog.h>
#include <QskWindow.h>
#include <QskControl.h>
#include <QskSetup.h>

#include <QGuiApplication>
#include <QHash>
#include <QPointer>

void qskSetLocale( QQuickItem* inputPanel, const QLocale& locale )
{
    if ( auto control = qobject_cast< QskControl* >( inputPanel ) )
    {
        control->setLocale( locale );
    }
    else
    {
        const auto mo = inputPanel->metaObject();

        const auto property = mo->property( mo->indexOfProperty( "locale" ) );
        if ( property.isWritable() )
            property.write( inputPanel, locale );
    }
}

QLocale qskLocale( const QQuickItem* inputPanel )
{
    if ( inputPanel == nullptr )
        return QLocale();

    if ( auto control = qobject_cast< const QskControl* >( inputPanel ) )
        return control->locale();

    return inputPanel->property( "locale" ).toLocale();
}

QskVirtualKeyboard* qskVirtualKeyboard( QQuickItem* inputPanel )
{
    // we should not depend on QskVirtualKeyboard TODO ...

    if ( inputPanel )
        return inputPanel->findChild< QskVirtualKeyboard* >();

    return nullptr;
}

class QskInputContext::PrivateData
{
public:
    PrivateData():
        ownsInputPanelWindow( false )
    {
    }

    QPointer< QQuickItem > inputItem;
    QPointer< QQuickItem > inputPanel;

    QskInputCompositionModel* compositionModel;
    QHash< QLocale, QskInputCompositionModel* > compositionModels;

    // the input panel is embedded in a window
    bool ownsInputPanelWindow : 1;
};

QskInputContext::QskInputContext():
    m_data( new PrivateData() )
{
    setObjectName( "InputContext" );

#if 1
    m_data->compositionModel = new QskInputCompositionModel( this );
#else
    m_data->compositionModel = new QskHunspellCompositionModel( this );
#endif

    connect( m_data->compositionModel, &QskInputCompositionModel::candidatesChanged,
        this, &QskInputContext::handleCandidatesChanged );

    connect( qskSetup, &QskSetup::inputPanelChanged,
        this, &QskInputContext::setInputPanel );

#if 1
    setCompositionModel( QLocale::Chinese, new QskPinyinCompositionModel( this ) );
#endif

    setInputPanel( qskSetup->inputPanel() );
}

QskInputContext::~QskInputContext()
{
#if 1
    if ( m_data->inputPanel )
        delete m_data->inputPanel;
#endif
}

bool QskInputContext::isValid() const
{
    return true;
}

bool QskInputContext::hasCapability( Capability ) const
{
    // what is QPlatformInputContext::HiddenTextCapability ???
    return true;
}

QQuickItem* QskInputContext::inputItem()
{
    return m_data->inputItem;
}

void QskInputContext::setInputItem( QQuickItem* item )
{
    if ( m_data->inputItem == item )
        return;

    m_data->inputItem = item;

    if ( m_data->inputItem == nullptr )
    {
        hideInputPanel();
        return;
    }

    update( Qt::ImQueryAll );
}

void QskInputContext::update( Qt::InputMethodQueries queries )
{
    if ( m_data->inputItem == nullptr || m_data->inputPanel == nullptr )
        return;

    const auto queryEvent = queryInputMethod( queries );

    if ( queryEvent.queries() & Qt::ImEnabled )
    {
        if ( !queryEvent.value( Qt::ImEnabled ).toBool() )
        {
            hideInputPanel();
            return;
        }
    }

    if ( queryEvent.queries() & Qt::ImHints )
    {
        /*
            ImhHiddenText = 0x1,          // might need to disable certain checks
            ImhSensitiveData = 0x2,       // shouldn't change anything
            ImhNoAutoUppercase = 0x4,     // if we support auto uppercase, disable it
            ImhPreferNumbers = 0x8,       // default to number keyboard
            ImhPreferUppercase = 0x10,    // start with shift on
            ImhPreferLowercase = 0x20,    // start with shift off
            ImhNoPredictiveText = 0x40,   // ignored for now

            ImhDate = 0x80,               // ignored for now (no date keyboard)
            ImhTime = 0x100,              // ignored for know (no time keyboard)

            ImhPreferLatin = 0x200,       // can be used to launch chinese kb in english mode

            ImhMultiLine = 0x400,         // not useful?

            ImhDigitsOnly                 // default to number keyboard, disable other keys
            ImhFormattedNumbersOnly       // hard to say
            ImhUppercaseOnly              // caps-lock, disable shift
            ImhLowercaseOnly              // disable shift
            ImhDialableCharactersOnly     // dial pad (calculator?)
            ImhEmailCharactersOnly        // disable certain symbols (email-only kb?)
            ImhUrlCharactersOnly          // disable certain symbols (url-only kb?)
            ImhLatinOnly                  // disable chinese input
         */

#if 0
        const auto hints = static_cast< Qt::InputMethodHints >(
            queryEvent.value( Qt::ImHints ).toInt() );

#endif
    }

    if ( queryEvent.queries() & Qt::ImPreferredLanguage )
    {
        const auto locale = queryEvent.value( Qt::ImPreferredLanguage ).toLocale();

        auto oldModel = compositionModel();

        if( m_data->inputPanel )
            qskSetLocale( m_data->inputPanel, locale );

        auto newModel = compositionModel();

        if( oldModel != newModel )
        {
            connect( newModel, &QskInputCompositionModel::candidatesChanged,
                this, &QskInputContext::handleCandidatesChanged );

            if ( auto keyboard = qskVirtualKeyboard( m_data->inputPanel ) )
                keyboard->setCandidateBarVisible( newModel->supportsSuggestions() );
        }
    }

    /*
        Qt::ImMicroFocus
        Qt::ImCursorRectangle
        Qt::ImFont
        Qt::ImCursorPosition
        Qt::ImSurroundingText // important for chinese input
        Qt::ImCurrentSelection // important for prediction
        Qt::ImMaximumTextLength // should be monitored
        Qt::ImAnchorPosition

        Qt::ImAbsolutePosition
        Qt::ImTextBeforeCursor // important for chinese
        Qt::ImTextAfterCursor  // important for chinese
        Qt::ImPlatformData     // hard to say...
        Qt::ImEnterKeyType
        Qt::ImAnchorRectangle
        Qt::ImInputItemClipRectangle // could be used for the geometry of the panel
     */
}

QRectF QskInputContext::keyboardRect() const
{
    if ( m_data->inputPanel
         && QskDialog::instance()->policy() != QskDialog::TopLevelWindow )
    {
        return qskItemGeometry( m_data->inputPanel );
    }

    return Inherited::keyboardRect();
}

bool QskInputContext::isAnimating() const
{
    return false;
}

void QskInputContext::showInputPanel()
{
    if ( !m_data->inputPanel )
    {
        setInputPanel( new QskVirtualKeyboard() );

        if ( QskDialog::instance()->policy() == QskDialog::TopLevelWindow )
        {
            m_data->ownsInputPanelWindow = true;

            auto window = new QskWindow;
            window->setFlags( Qt::Tool | Qt::WindowDoesNotAcceptFocus );
            window->resize( 800, 240 ); // ### what size?

            m_data->inputPanel->setParentItem( window->contentItem() );
            connect( window, &QskWindow::visibleChanged,
                this, &QskInputContext::emitInputPanelVisibleChanged );
        }
        else
        {
            auto window = qobject_cast< QQuickWindow* >( QGuiApplication::focusWindow() );
            if ( window )
            {
                m_data->inputPanel->setParentItem( window->contentItem() );
                m_data->inputPanel->setSize( window->size() );
            }
        }
    }

    if ( m_data->ownsInputPanelWindow )
    {
        if ( m_data->inputPanel->window() )
            m_data->inputPanel->window()->show();
    }
    else
    {
        m_data->inputPanel->setVisible( true );
    }
}

void QskInputContext::hideInputPanel()
{
    if ( m_data->inputPanel == nullptr )
        return;

    if ( m_data->ownsInputPanelWindow )
    {
        if ( auto window = m_data->inputPanel->window() )
            window->hide();
    }
    else
    {
        m_data->inputPanel->setVisible( false );
    }
}

bool QskInputContext::isInputPanelVisible() const
{
    auto panel = m_data->inputPanel;

    return panel && panel->isVisible()
        && panel->window() && panel->window()->isVisible();
}

QLocale QskInputContext::locale() const
{
    return qskLocale( m_data->inputPanel );
}

Qt::LayoutDirection QskInputContext::inputDirection() const
{
    return Inherited::inputDirection();
}

void QskInputContext::setFocusObject( QObject* focusObject )
{
    auto focusItem = qobject_cast< QQuickItem* >( focusObject );

    if ( focusItem == nullptr )
    {
        if ( m_data->inputItem )
        {
            if ( m_data->inputItem->window() == QGuiApplication::focusWindow() )
                setInputItem( nullptr );
        }
    }
    else
    {
        /*
            Do not change the input item when
            navigating on or into the panel
         */

        if( qskNearestFocusScope( focusItem ) != m_data->inputPanel )
            setInputItem( focusItem );
    }
}

void QskInputContext::setCompositionModel(
    const QLocale& locale, QskInputCompositionModel* model )
{
    auto& models = m_data->compositionModels;

    if ( model )
    {
        const auto it = models.find( locale );
        if ( it != models.end() )
        {
            if ( it.value() == model )
                return;

            delete it.value();
            *it = model;
        }
        else
        {
            models.insert( locale, model );
        }

        connect( model, &QskInputCompositionModel::candidatesChanged,
            this, &QskInputContext::handleCandidatesChanged );
    }
    else
    {
        const auto it = models.find( locale );
        if ( it != models.end() )
        {
            delete it.value();
            models.erase( it );
        }
    }
}

QskInputCompositionModel* QskInputContext::compositionModel() const
{
    return m_data->compositionModels.value( locale(), m_data->compositionModel );
}

void QskInputContext::invokeAction( QInputMethod::Action action, int value )
{
    auto model = compositionModel();

    switch ( static_cast< int >( action ) )
    {
        case QskVirtualKeyboard::Compose:
        {
            model->composeKey( static_cast< Qt::Key >( value ) );
            break;
        }
        case QskVirtualKeyboard::SelectCandidate:
        {
            model->commitCandidate( value );
            break;
        }
        case QInputMethod::Click:
        case QInputMethod::ContextMenu:
        {
            break;
        }
    }
}

void QskInputContext::handleCandidatesChanged()
{
    const auto model = compositionModel();
    if ( model == nullptr || m_data->inputPanel == nullptr )
        return;

    const auto count = model->candidateCount();

    QVector< QString > candidates;
    candidates.reserve( count );

    for( int i = 0; i < count; i++ )
        candidates += model->candidate( i );

    if ( auto keyboard = qskVirtualKeyboard( m_data->inputPanel ) )
        keyboard->setPreeditCandidates( candidates );
}

void QskInputContext::setInputPanel( QQuickItem* inputPanel )
{
    if ( m_data->inputPanel == inputPanel )
        return;

    auto model = compositionModel();

    if ( m_data->inputPanel )
    {
        m_data->inputPanel->disconnect( this );

        if ( model )
            model->disconnect( m_data->inputPanel );
    }

    m_data->inputPanel = inputPanel;
    m_data->ownsInputPanelWindow = false;

    if ( inputPanel )
    {
        // maybe using a QQuickItemChangeListener instead
#if 1
        connect( inputPanel, &QQuickItem::visibleChanged,
            this, &QPlatformInputContext::emitInputPanelVisibleChanged );

        connect( inputPanel, &QQuickItem::xChanged,
            this, &QPlatformInputContext::emitKeyboardRectChanged );

        connect( inputPanel, &QQuickItem::yChanged,
            this, &QPlatformInputContext::emitKeyboardRectChanged );

        connect( inputPanel, &QQuickItem::widthChanged,
            this, &QPlatformInputContext::emitKeyboardRectChanged );

        connect( inputPanel, &QQuickItem::heightChanged,
            this, &QPlatformInputContext::emitKeyboardRectChanged );
#endif

        if ( auto control = qobject_cast< QskControl* >( inputPanel ) )
        {
            connect( control, &QskControl::localeChanged,
                this, &QPlatformInputContext::emitLocaleChanged );
        }

        if ( model )
        {
            if ( auto keyboard = qskVirtualKeyboard( inputPanel ) )
                keyboard->setCandidateBarVisible( model->supportsSuggestions() );
        }
    }
}

void QskInputContext::reset()
{
}

void QskInputContext::commit()
{
}

bool QskInputContext::filterEvent( const QEvent* event )
{
    // called from QXcbKeyboard, but what about other platforms
    Q_UNUSED( event )
    return false;
}

QInputMethodQueryEvent QskInputContext::queryInputMethod(
    Qt::InputMethodQueries queries ) const
{
    QInputMethodQueryEvent event( queries );
    sendEventToInputItem( &event );

    return event;
}

void QskInputContext::sendEventToInputItem( QEvent* event ) const
{
    if ( m_data->inputItem && event )
        QCoreApplication::sendEvent( m_data->inputItem, event );
}

#include "moc_QskInputContext.cpp"
