/******************************************************************************
 * QSkinny - Copyright (C) 2016 Uwe Rathmann
 * This file may be used under the terms of the QSkinny License, Version 1.0
 *****************************************************************************/

#include "QskTextInput.h"

// VS2012+ disable keyword macroizing unless _ALLOW_KEYWORD_MACROS is set
#ifdef _MSC_VER
#if ( _MSC_VER >= 1700 ) && !defined( _ALLOW_KEYWORD_MACROS )
#define _ALLOW_KEYWORD_MACROS
#endif
#endif

QSK_QT_PRIVATE_BEGIN
#define private public
#include <private/qquicktextinput_p.h>
#include <private/qquicktextinput_p_p.h>
#undef private
QSK_QT_PRIVATE_END

static inline void qskBindSignals( const QQuickTextInput* wrappedInput,
    QskTextInput* input )
{
    QObject::connect( wrappedInput, &QQuickTextInput::textChanged,
        input, [ input ] { input->Q_EMIT textChanged( input->text() ); } );

#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    QObject::connect( wrappedInput, &QQuickTextInput::textEdited,
        input, [ input ] { input->Q_EMIT textEdited( input->text() ); } );
#endif

    QObject::connect( wrappedInput, &QQuickTextInput::textChanged,
        input, [ input ] { input->Q_EMIT textChanged( input->text() ); } );

    QObject::connect( wrappedInput, &QQuickTextInput::selectedTextChanged,
        input, [ input ] { input->Q_EMIT selectedTextChanged( input->selectedText() ); } );

    QObject::connect( wrappedInput, &QQuickTextInput::validatorChanged,
        input, &QskTextInput::validatorChanged );

    QObject::connect( wrappedInput, &QQuickTextInput::inputMaskChanged,
        input, &QskTextInput::inputMaskChanged );

    QObject::connect( wrappedInput, &QQuickTextInput::readOnlyChanged,
        input, &QskTextInput::readOnlyChanged );

#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    QObject::connect( wrappedInput, &QQuickTextInput::overwriteModeChanged,
        input, &QskTextInput::overwriteModeChanged );
#endif

    QObject::connect( wrappedInput, &QQuickTextInput::maximumLengthChanged,
        input, &QskTextInput::maximumLengthChanged );

    QObject::connect( wrappedInput, &QQuickTextInput::echoModeChanged,
        input, [ input ] { input->Q_EMIT echoModeChanged( input->echoMode() ); } );

    QObject::connect( wrappedInput, &QQuickTextInput::autoScrollChanged,
        input, &QskTextInput::autoScrollChanged );

    QObject::connect( wrappedInput, &QQuickTextInput::selectByMouseChanged,
        input, &QskTextInput::selectByMouseChanged );

    QObject::connect( wrappedInput, &QQuickTextInput::persistentSelectionChanged,
        input, &QskTextInput::persistentSelectionChanged );

    QObject::connect( wrappedInput, &QQuickItem::implicitWidthChanged,
        input, &QskControl::resetImplicitSize );

    QObject::connect( wrappedInput, &QQuickItem::implicitHeightChanged,
        input, &QskControl::resetImplicitSize );
}

namespace
{
    class TextInput final : public QQuickTextInput
    {
    public:
        TextInput( QQuickItem* parent ):
            QQuickTextInput( parent )
        {
            classBegin();

            setActiveFocusOnTab( false );
            setFlag( ItemAcceptsInputMethod, false );
            setFocusOnPress( false );

            connect( this, &TextInput::contentSizeChanged,
                this, &TextInput::updateClip );

            componentComplete();
        }

        void setAlignment( Qt::Alignment alignment )
        {
            setHAlign( ( HAlignment ) ( int( alignment ) & 0x0f ) );
            setVAlign( ( VAlignment ) ( int( alignment ) & 0xf0 ) );
        }

        inline bool handleEvent( QEvent* event )
        {
            return QQuickTextInput::event( event );
        }

        virtual void focusInEvent( QFocusEvent* ) override
        {
            auto d = QQuickTextInputPrivate::get( this );

            if ( d->m_readOnly )
                return;

            d->cursorVisible = true;

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
            d->updateCursorBlinking();
            d->setBlinkingCursorEnabled( true );
#endif

            if ( d->determineHorizontalAlignment() )
            {
                d->updateLayout();
                d->updateHorizontalScroll();
                d->updateVerticalScroll();

#if 0
                updateInputMethod(Qt::ImCursorRectangle | Qt::ImAnchorRectangle);
#endif
            }

            connect( QGuiApplication::inputMethod(),
                SIGNAL(inputDirectionChanged(Qt::LayoutDirection)),
                this, SLOT(q_updateAlignment()) );

            qGuiApp->inputMethod()->show();
        }

        virtual void focusOutEvent( QFocusEvent* event ) override
        {
            auto d = QQuickTextInputPrivate::get( this );

            if (d->m_readOnly)
                return;

            d->cursorVisible = false;

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
            d->updateCursorBlinking();
            d->setBlinkingCursorEnabled( false );
#endif

            if ( d->m_passwordEchoEditing || d->m_passwordEchoTimer.isActive() )
            {
                d->updatePasswordEchoEditing( false );
            }

            if ( event->reason() != Qt::ActiveWindowFocusReason
                && event->reason() != Qt::PopupFocusReason )
            {
                if ( d->hasSelectedText() && !d->persistentSelection )
                    deselect();
            }

            const auto status = d->hasAcceptableInput( d->m_text );
            if ( status == QQuickTextInputPrivate::AcceptableInput )
            {
                if ( d->fixup() )
                    Q_EMIT editingFinished();
            }

            disconnect( QGuiApplication::inputMethod(),
                SIGNAL(inputDirectionChanged(Qt::LayoutDirection)),
                this, SLOT(q_updateAlignment()) );
        }

        virtual void geometryChanged(
            const QRectF& newGeometry, const QRectF& oldGeometry ) override
        {
            QQuickTextInput::geometryChanged( newGeometry, oldGeometry );
            updateClip();
        }

        void updateClip()
        {
            setClip( ( contentWidth() > width() ) ||
                ( contentHeight() > height() ) );
        }
    };
}

QSK_SUBCONTROL( QskTextInput, Panel )
QSK_SUBCONTROL( QskTextInput, Text )

class QskTextInput::PrivateData
{
public:
    TextInput* textInput;
};

QskTextInput::QskTextInput( QQuickItem* parent ):
    Inherited( parent ),
    m_data( new PrivateData() )
{
    setPolishOnResize( true );
    setFocusPolicy( Qt::StrongFocus );

    setFlag( QQuickItem::ItemAcceptsInputMethod );

    /*
        QQuickTextInput is a beast of almost 5k lines of code, we don't
        want to reimplement that - at least not now.
        So this is more or less a simple wrapper making everything
        conforming to qskinny.
     */

    m_data->textInput = new TextInput( this );
    qskBindSignals( m_data->textInput, this );

    setAcceptedMouseButtons( m_data->textInput->acceptedMouseButtons() );
    m_data->textInput->setAcceptedMouseButtons( Qt::NoButton );

    initSizePolicy( QskSizePolicy::Minimum, QskSizePolicy::Fixed );
}

QskTextInput::QskTextInput( const QString& text, QQuickItem* parent ):
    QskTextInput( parent )
{
    m_data->textInput->setText( text );
}

QskTextInput::~QskTextInput()
{
}

bool QskTextInput::event( QEvent* event )
{
    if ( event->type() == QEvent::ShortcutOverride )
    {
        return m_data->textInput->handleEvent( event );
    }
    else if ( event->type() == QEvent::LocaleChange )
    {
        qskUpdateInputMethod( this, Qt::ImPreferredLanguage );
    }

    return Inherited::event( event );
}

void QskTextInput::keyPressEvent( QKeyEvent* event )
{
    m_data->textInput->handleEvent( event );
}

void QskTextInput::keyReleaseEvent( QKeyEvent* event )
{
    Inherited::keyReleaseEvent( event );
}

void QskTextInput::mousePressEvent( QMouseEvent* event )
{
    m_data->textInput->handleEvent( event );

    if ( !isReadOnly() && !qGuiApp->styleHints()->setFocusOnTouchRelease() )
        qGuiApp->inputMethod()->show();
}

void QskTextInput::mouseMoveEvent( QMouseEvent* event )
{
    m_data->textInput->handleEvent( event );
}

void QskTextInput::mouseReleaseEvent( QMouseEvent* event )
{
    m_data->textInput->handleEvent( event );

    if ( !isReadOnly() && qGuiApp->styleHints()->setFocusOnTouchRelease() )
        qGuiApp->inputMethod()->show();
}

void QskTextInput::mouseDoubleClickEvent( QMouseEvent* event )
{
    m_data->textInput->handleEvent( event );
}

void QskTextInput::inputMethodEvent( QInputMethodEvent* event )
{
    m_data->textInput->handleEvent( event );
}

void QskTextInput::focusInEvent( QFocusEvent* event )
{
    m_data->textInput->handleEvent( event );
    Inherited::focusInEvent( event );
}

void QskTextInput::focusOutEvent( QFocusEvent* event )
{
    m_data->textInput->handleEvent( event );
    Inherited::focusOutEvent( event );
}

QSizeF QskTextInput::contentsSizeHint() const
{
    using namespace QskAspect;

    const qreal w = m_data->textInput->implicitWidth();
    const qreal h = m_data->textInput->implicitHeight();

    const QSizeF minSize( metric( Panel | MinimumWidth ),
        metric( Panel | MinimumHeight ) );

    return outerBoxSize( Panel, QSizeF( w, h ) ).expandedTo( minSize );
}

void QskTextInput::updateLayout()
{
    auto input = m_data->textInput;

    input->setAlignment( alignment() );
    input->setFont( font() );

    qskSetItemGeometry( input, subControlRect( Text ) );
}

QString QskTextInput::text() const
{
    return m_data->textInput->text();
}

void QskTextInput::setText( const QString& text )
{
    m_data->textInput->setText( text );
}

int QskTextInput::fontRole() const
{
    return QskSkinnable::fontRole( Text );
}

void QskTextInput::setFontRole( int role )
{
    const int oldRole = fontRole();

    QskSkinnable::setFontRole( effectiveSubcontrol( Text ), role );

    if ( oldRole != role )
    {
        polish();
        resetImplicitSize();

        Qt::InputMethodQueries queries = Qt::ImCursorRectangle | Qt::ImFont;
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        queries |= Qt::ImAnchorRectangle;
#endif

        qskUpdateInputMethod( this, queries );
        Q_EMIT fontRoleChanged();
    }
}

void QskTextInput::setAlignment( Qt::Alignment alignment )
{
    if ( alignment != this->alignment() )
    {
        const auto subControl = effectiveSubcontrol( Text );
        setFlagHint( subControl | QskAspect::Alignment, alignment );

        m_data->textInput->setAlignment( alignment );

        polish();

        Q_EMIT alignmentChanged();
    }
}

Qt::Alignment QskTextInput::alignment() const
{
    return flagHint< Qt::Alignment >(
        Text | QskAspect::Alignment, Qt::AlignLeft | Qt::AlignTop );
}

QFont QskTextInput::font() const
{
    return effectiveFont( QskTextInput::Text );
}

bool QskTextInput::isReadOnly() const
{
    return m_data->textInput->isReadOnly();
}

void QskTextInput::setReadOnly( bool on )
{
    m_data->textInput->setReadOnly( on );
    m_data->textInput->setFlag( QQuickItem::ItemAcceptsInputMethod, false );

    qskUpdateInputMethod( this, Qt::ImEnabled );
}

void QskTextInput::ensureVisible( int position )
{
    m_data->textInput->ensureVisible( position );
}

bool QskTextInput::isCursorVisible() const
{
    return m_data->textInput->isCursorVisible();
}

void QskTextInput::setCursorVisible( bool on )
{
    m_data->textInput->setCursorVisible( on );
}

int QskTextInput::cursorPosition() const
{
    return m_data->textInput->cursorPosition();
}

void QskTextInput::setCursorPosition(int pos)
{
    m_data->textInput->setCursorPosition( pos );
}

int QskTextInput::selectionStart() const
{
    return m_data->textInput->selectionStart();
}

int QskTextInput::selectionEnd() const
{
    return m_data->textInput->selectionEnd();
}

QString QskTextInput::selectedText() const
{
    return m_data->textInput->selectedText();
}

int QskTextInput::maxLength() const
{
    return m_data->textInput->maxLength();
}

void QskTextInput::setMaxLength(int length)
{
    m_data->textInput->setMaxLength( length );
}

QValidator* QskTextInput::validator() const
{
    return m_data->textInput->validator();
}

void QskTextInput::setValidator( QValidator* validator )
{
    m_data->textInput->setValidator( validator );
}

QString QskTextInput::inputMask() const
{
    return m_data->textInput->inputMask();
}

void QskTextInput::setInputMask( const QString& mask )
{
    m_data->textInput->setInputMask( mask );
}

QskTextInput::EchoMode QskTextInput::echoMode() const
{
    const auto mode = m_data->textInput->echoMode();
    return static_cast< QskTextInput::EchoMode >( mode );
}

void QskTextInput::setEchoMode( EchoMode mode )
{
    m_data->textInput->setEchoMode(
        static_cast< QQuickTextInput::EchoMode >( mode ) );

    qskUpdateInputMethod( this, Qt::ImHints );
}

QString QskTextInput::displayText() const
{
    return m_data->textInput->displayText();
}

QString QskTextInput::preeditText() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    return m_data->textInput->preeditText();
#else
    return QString();
#endif
}

bool QskTextInput::overwriteMode() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    return m_data->textInput->overwriteMode();
#else
    return false;
#endif
}

void QskTextInput::setOverwriteMode( bool overwrite )
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    m_data->textInput->setOverwriteMode( overwrite );
#else
    Q_UNUSED( overwrite )
#endif
}

bool QskTextInput::autoScroll() const
{
    return m_data->textInput->autoScroll();
}

void QskTextInput::setAutoScroll(bool on)
{
    m_data->textInput->setAutoScroll( on );
}

bool QskTextInput::selectByMouse() const
{
    return m_data->textInput->selectByMouse();
}

void QskTextInput::setSelectByMouse(bool on)
{
    m_data->textInput->setSelectByMouse( on );
}

QskTextInput::SelectionMode QskTextInput::mouseSelectionMode() const
{
    const auto mode = m_data->textInput->mouseSelectionMode();
    return static_cast< SelectionMode >( mode );
}

void QskTextInput::setMouseSelectionMode( SelectionMode mode )
{
    m_data->textInput->setMouseSelectionMode(
        static_cast< QQuickTextInput::SelectionMode >( mode ) );
}

bool QskTextInput::persistentSelection() const
{
    return m_data->textInput->persistentSelection();
}

void QskTextInput::setPersistentSelection(bool persist)
{
    m_data->textInput->setPersistentSelection( persist );
}

bool QskTextInput::hasAcceptableInput() const
{
    return m_data->textInput->hasAcceptableInput();
}

QVariant QskTextInput::inputMethodQuery(
    Qt::InputMethodQuery property) const
{
    return inputMethodQuery( property, QVariant() );
}

QVariant QskTextInput::inputMethodQuery(
    Qt::InputMethodQuery query, QVariant argument) const
{
    switch( query )
    {
        case Qt::ImEnabled:
        {
            return QVariant( (bool)( flags() & ItemAcceptsInputMethod ) );
        }
        case Qt::ImFont:
        {
            return font();
        }
        case Qt::ImPreferredLanguage:
        {
            return locale();
        }
        case Qt::ImCursorRectangle:
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        case Qt::ImInputItemClipRectangle:
#endif
        {
            QVariant v = m_data->textInput->inputMethodQuery( query, argument );
#if 1
            if ( v.canConvert< QRectF >() )
                v.setValue( v.toRectF().translated( m_data->textInput->position() ) );
#endif
            return v;
        }
        default:
        {
            return m_data->textInput->inputMethodQuery( query, argument );
        }
    }
}

bool QskTextInput::canUndo() const
{
    return m_data->textInput->canUndo();
}

bool QskTextInput::canRedo() const
{
    return m_data->textInput->canRedo();
}

bool QskTextInput::isInputMethodComposing() const
{
    return m_data->textInput->isInputMethodComposing();
}

Qt::InputMethodHints QskTextInput::inputMethodHints() const
{
    return m_data->textInput->inputMethodHints();
}

void QskTextInput::setInputMethodHints(Qt::InputMethodHints hints )
{
    if ( m_data->textInput->inputMethodHints() != hints )
    {
        m_data->textInput->setInputMethodHints( hints );
        qskUpdateInputMethod( this, Qt::ImHints );
    }
}

#include "moc_QskTextInput.cpp"
