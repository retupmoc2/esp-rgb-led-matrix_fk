/* MIT License
 *
 * Copyright (c) 2019 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
    DESCRIPTION
*******************************************************************************/
/**
 * @brief  Display manager
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "DisplayMgr.h"
#include "Board.h"
#include "LedMatrix.h"
#include "AmbientLightSensor.h"

#include <TextWidget.h>
#include <BitmapWidget.h>
#include <LampWidget.h>
#include <Logging.h>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/** Get number of array elements. */
#define ARRAY_NUM(__arr)    (sizeof(__arr) / sizeof((__arr)[0]))

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/* Initialize LED matrix instance. */
DisplayMgr  DisplayMgr::m_instance;

/* Initialize text widget name. */
const char* DisplayMgr::TEXT_WIDGET_NAME    = "text";

/* Initialize bitmap widget name. */
const char* DisplayMgr::BMP_WIDGET_NAME     = "bitmap";

/* Initialize lamp widget name. */
const char* DisplayMgr::LAMP_WIDGET_NAME    = "lamp";

/******************************************************************************
 * Public Methods
 *****************************************************************************/

void DisplayMgr::init(void)
{
    uint16_t brightness = (static_cast<uint16_t>(UINT8_MAX) * BRIGHTNESS_DEFAULT) / 100u;

    /* Set the display brightness here just once.
     * There is no need to do this in the process() method periodically.
     */
    LedMatrix::getInstance().setBrightness(brightness);

    return;
}

bool DisplayMgr::enableAutoBrightnessAdjustment(bool enable)
{
    bool status = true;

    /* Disable automatic brightness adjustment? */
    if (false == enable)
    {
        m_autoBrightnessTimer.stop();
    }
    /* Enable automatic brightness adjustment */
    else
    {
        /* If no ambient light sensor is available, enable it makes no sense. */
        if (false == AmbientLightSensor::getInstance().isSensorAvailable())
        {
            status = false;
        }
        /* Ambient light sensor is available */
        else
        {
            /* Display brightness will be automatically adjusted in the process() method. */
            m_autoBrightnessTimer.start(ALS_AUTO_ADJUST_PERIOD);
        }
    }

    return status;
}

Canvas* DisplayMgr::getSlot(uint8_t slotId)
{
    Canvas* slotCanvas = NULL;

    if (MAX_SLOTS > slotId)
    {
        slotCanvas = m_slots[slotId];
    }
    
    return slotCanvas;
}

void DisplayMgr::setLayout(uint8_t slotId, LayoutId layoutId)
{
    Widget* widget = NULL;

    if ((MAX_SLOTS <= slotId) ||
        (LAYOUT_ID_COUNT <= layoutId))
    {
        return;
    }

    widget = m_slots[slotId];
    destroyWidget(widget);

    switch(layoutId)
    {
    case LAYOUT_ID_0:
        if (false == createLayout0(m_slots[slotId]))
        {
            LOG_WARNING(String("Couldn't create layout 0 for slot ") + slotId);
        }
        break;

    case LAYOUT_ID_1:
        if (false == createLayout1(m_slots[slotId], &m_bitmapBuffer[slotId][0]))
        {
            LOG_WARNING(String("Couldn't create layout 1 for slot ") + slotId);
        }
        break;

    case LAYOUT_ID_2:
        if (false == createLayout2(m_slots[slotId], &m_bitmapBuffer[slotId][0]))
        {
            LOG_WARNING(String("Couldn't create layout 2 for slot ") + slotId);
        }
        break;

    case LAYOUT_ID_COUNT:
        /* Don't care about it. */
        break;

    default:
        /* Don't care about it. */
        break;
    }

    return;
}

void DisplayMgr::setText(uint8_t slotId, const String& str)
{
    Widget* widget = NULL;

    if ((MAX_SLOTS > slotId) &&
        (NULL != m_slots[slotId]))
    {
        widget = m_slots[slotId]->find(TEXT_WIDGET_NAME);
    }

    if (NULL != widget)
    {
        TextWidget* textWidget = NULL;

        if (0 == strcmp(widget->getType(), TextWidget::WIDGET_TYPE))
        {
            textWidget = static_cast<TextWidget*>(widget);

            textWidget->setStr(str);
        }
    }

    return;
}

void DisplayMgr::setBitmap(uint8_t slotId, const uint16_t* bitmap, uint16_t width, uint16_t height)
{
    Widget* widget = NULL;

    if ((MAX_SLOTS > slotId) &&
        (NULL != m_slots[slotId]))
    {
        widget = m_slots[slotId]->find(BMP_WIDGET_NAME);
    }

    if (NULL != widget)
    {
        BitmapWidget* bitmapWidget = NULL;

        if (0 == strcmp(widget->getType(), BitmapWidget::WIDGET_TYPE))
        {
            bitmapWidget = static_cast<BitmapWidget*>(widget);

            if ((BMP_WIDTH >= width) &&
                (BMP_HEIGHT >= height))
            {
                memcpy(&m_bitmapBuffer[slotId][0], bitmap, sizeof(uint16_t) * width * height);
                bitmapWidget->set(&m_bitmapBuffer[slotId][0], width, height);
            }
        }
    }

    return;
}

void DisplayMgr::setLamp(uint8_t slotId, uint8_t lampId, bool onState)
{
    Widget* widget      = NULL;
    String  widgetName  = LAMP_WIDGET_NAME;
    
    widgetName += lampId;

    if ((MAX_SLOTS > slotId) &&
        (NULL != m_slots[slotId]))
    {
        widget = m_slots[slotId]->find(widgetName.c_str());
    }

    if (NULL != widget)
    {
        LampWidget* lampWidget = NULL;

        if (0 == strcmp(widget->getType(), LampWidget::WIDGET_TYPE))
        {
            lampWidget = static_cast<LampWidget*>(widget);

            lampWidget->setOnState(onState);
        }
    }

    return;
}

void DisplayMgr::setAllLamps(uint8_t slotId, bool onState)
{
    if ((MAX_SLOTS > slotId) &&
        (NULL != m_slots[slotId]))
    {
        Widget* widget      = NULL;
        String  widgetName;
        uint8_t lampId      = 0u;

        do
        {
            widgetName  = LAMP_WIDGET_NAME;
            widgetName  += lampId;
            widget      = m_slots[slotId]->find(widgetName.c_str());

            if (NULL != widget)
            {
                LampWidget* lampWidget = NULL;

                if (0 == strcmp(widget->getType(), LampWidget::WIDGET_TYPE))
                {
                    lampWidget = static_cast<LampWidget*>(widget);

                    lampWidget->setOnState(onState);
                }
            }
        }
        while(NULL != widget);

    }

    return;
}

void DisplayMgr::process(void)
{
    LedMatrix& matrix = LedMatrix::getInstance();

    matrix.clear();

    /* If the slot mechanism is disabled, only system messages are shown. */
    if (false == m_slotsEnabled)
    {
        m_sysMsgWidget.update(matrix);
    }
    /* Slot mechanism enabled. */
    else
    {
        /* Slot rotation enabled? */
        if (true == m_slotChangeTimer.isTimerRunning())
        {
            /* Jump to next slot? */
            if (true == m_slotChangeTimer.isTimeout())
            {
                /* Set next slot active */
                ++m_activeSlotId;
                m_activeSlotId %= MAX_SLOTS;

                LOG_INFO(String("Next slot: ") + m_activeSlotId);

                /* Wait another period */
                m_slotChangeTimer.restart();
            }
        }

        /* Anything to show? */
        if (NULL != m_slots[m_activeSlotId])
        {
            m_slots[m_activeSlotId]->update(matrix);
        }
    }

    /* Ambient light sensor available for automatic brightness adjustment? */
    if (true == m_autoBrightnessTimer.isTimerRunning())
    {
        float   lightNormalized         = AmbientLightSensor::getInstance().getNormalizedLight();
        uint8_t BRIGHTNESS_DYN_RANGE    = UINT8_MAX - BRIGHTNESS_MIN;
        float   fBrightness             = static_cast<float>(BRIGHTNESS_MIN) + ( static_cast<float>(BRIGHTNESS_DYN_RANGE) * lightNormalized );
        uint8_t brightness              = static_cast<uint8_t>(fBrightness);

        matrix.setBrightness(brightness);

        m_autoBrightnessTimer.restart();
    }

    matrix.show();

    return;
}

void DisplayMgr::startRotating(bool start)
{
    m_rotate        = start;
    m_activeSlotId  = 0;

    if (false == start)
    {
        m_slotChangeTimer.stop();
    }
    else
    {
        m_slotChangeTimer.start(DEFAULT_PERIOD);
    }
    
    return;
}

void DisplayMgr::enableSlots(bool enableIt)
{
    m_slotsEnabled = enableIt;

    return;
}

void DisplayMgr::showSysMsg(const String& msg)
{
    m_sysMsgWidget.setStr(msg);
    enableSlots(false);

    /* Show message instant */
    process();
    
    return;
}

void DisplayMgr::delay(uint32_t waitTime)
{
    uint32_t start = millis();

    while(waitTime > (millis() - start))
    {
        process();
    }

    return;
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

DisplayMgr::DisplayMgr() :
    m_slots(),
    m_activeSlotId(0u),
    m_slotChangeTimer(),
    m_rotate(false),
    m_slotsEnabled(false),
    m_sysMsgWidget(),
    m_bitmapBuffer(),
    m_autoBrightnessTimer()
{
    uint8_t index = 0u;

    /* Initialize all slots */
    for(index = 0u; index < MAX_SLOTS; ++index)
    {
        m_slots[index] = NULL;
    }
}

DisplayMgr::~DisplayMgr()
{
    /* Destroy the widgets of all slots */
    clearSlots();
}

void DisplayMgr::destroyWidget(Widget*& widget)
{
    if (NULL != widget)
    {
        /* If the widget is a canvas, its children will be destroyed as well. */
        if (0 == strcmp(Canvas::WIDGET_TYPE, widget->getType()))
        {
            Canvas*             canvas      = static_cast<Canvas*>(widget);
            LinkedList<Widget*> children    = canvas->children();
            
            if (true == children.selectFirstElement())
            {
                do
                {
                    destroyWidget(*children.current());

                } while (true == children.next());
            }
        }
        else
        {
            delete widget;
            widget = NULL;
        }
    }

    return;
}

void DisplayMgr::clearSlots(void)
{
    uint8_t index = 0u;

    for(index = 0u; index < ARRAY_NUM(m_slots); ++index)
    {
        Widget* widget = m_slots[index];
        destroyWidget(widget);
    }

    return;
}

bool DisplayMgr::createLayout0(Canvas*& canvas) const
{
    bool        success     = true;
    TextWidget* textWidget  = new TextWidget();

    canvas = new Canvas(Board::LedMatrix::width, Board::LedMatrix::height, 0, 0);

    if ((NULL == canvas) ||
        (NULL == textWidget))
    {
        success = false;
    }
    else if (false == canvas->addWidget(*textWidget))
    {
        success = false;
    }
    else
    {
        textWidget->setName(TEXT_WIDGET_NAME);
    }
    
    if (false == success)
    {
        if (NULL != canvas)
        {
            delete canvas;
            canvas = NULL;
        }

        if (NULL != textWidget)
        {
            delete textWidget;
        }
    }

    return success;
}

bool DisplayMgr::createLayout1(Canvas*& canvas, uint16_t* bitmapBuffer) const
{
    bool            success         = true;
    const uint16_t  TEXT_WIDTH      = Board::LedMatrix::width - BMP_WIDTH - 1u;
    const uint16_t  TEXT_HEIGHT     = Board::LedMatrix::height;
    Canvas*         bitmapCanvas    = new Canvas(BMP_WIDTH, BMP_HEIGHT, 0, 0);
    BitmapWidget*   bitmapWidget    = new BitmapWidget();
    Canvas*         textCanvas      = new Canvas(TEXT_WIDTH, TEXT_HEIGHT, BMP_WIDTH, 0);
    TextWidget*     textWidget      = new TextWidget();

    canvas = new Canvas(Board::LedMatrix::width, Board::LedMatrix::height, 0, 0);

    if ((NULL == canvas) ||
        (NULL == bitmapCanvas) ||
        (NULL == bitmapWidget) ||
        (NULL == textCanvas) ||
        (NULL == textWidget))
    {
        success = false;
    }
    else if (false == canvas->addWidget(*bitmapCanvas))
    {
        success = false;
    }
    else if (false == canvas->addWidget(*textCanvas))
    {
        success = false;
    }
    else if (false == bitmapCanvas->addWidget(*bitmapWidget))
    {
        success = false;
    }
    else if (false == textCanvas->addWidget(*textWidget))
    {
        success = false;
    }
    else
    {
        bitmapWidget->setName(BMP_WIDGET_NAME);
        bitmapWidget->set(bitmapBuffer, BMP_WIDTH, BMP_HEIGHT);

        textWidget->setName(TEXT_WIDGET_NAME);
    }

    if (false == success)
    {
        if (NULL != canvas)
        {
            delete canvas;
            canvas = NULL;
        }

        if (NULL != bitmapCanvas)
        {
            delete bitmapCanvas;
        }

        if (NULL != bitmapWidget)
        {
            delete bitmapWidget;
        }

        if (NULL != textCanvas)
        {
            delete textCanvas;
        }

        if (NULL != textWidget)
        {
            delete textWidget;
        }
    }

    return success;
}

bool DisplayMgr::createLayout2(Canvas*& canvas, uint16_t* bitmapBuffer) const
{
    bool            success         = true;
    const uint16_t  TEXT_WIDTH      = Board::LedMatrix::width - BMP_WIDTH - 1u;
    const uint16_t  TEXT_HEIGHT     = 5u;
    const uint16_t  LAMP_WIDTH      = Board::LedMatrix::width - BMP_WIDTH;
    const uint16_t  LAMP_HEIGHT     = 1u;
    Canvas*         bitmapCanvas    = new Canvas(BMP_WIDTH, BMP_HEIGHT, 0, 0);
    BitmapWidget*   bitmapWidget    = new BitmapWidget();
    Canvas*         textCanvas      = new Canvas(TEXT_WIDTH, TEXT_HEIGHT, static_cast<int16_t>(BMP_WIDTH) + 1, 1);
    TextWidget*     textWidget      = new TextWidget();
    Canvas*         lampCanvas      = new Canvas(LAMP_WIDTH, LAMP_HEIGHT, static_cast<int16_t>(BMP_WIDTH), 0);
    LampWidget*     lampWidgets[]   = { NULL, NULL, NULL, NULL };
    uint8_t         index           = 0u;
    char            lampName[strlen(LAMP_WIDGET_NAME) + 2];
    uint8_t         lampNameNumPos  = 0;

    canvas = new Canvas(Board::LedMatrix::width, Board::LedMatrix::height, 0, 0);

    for(index = 0u; index < ARRAY_NUM(lampWidgets); ++index)
    {
        lampWidgets[index] = new LampWidget();

        if (NULL == lampWidgets[index])
        {
            break;
        }
    }

    if ((ARRAY_NUM(lampWidgets) > index) ||
        (NULL == canvas) ||
        (NULL == bitmapCanvas) ||
        (NULL == bitmapWidget) ||
        (NULL == textCanvas) ||
        (NULL == textWidget))
    {
        success = false;
    }
    else if (false == canvas->addWidget(*bitmapCanvas))
    {
        success = false;
    }
    else if (false == canvas->addWidget(*textCanvas))
    {
        success = false;
    }
    else if (false == canvas->addWidget(*lampCanvas))
    {
        success = false;
    }
    else if (false == bitmapCanvas->addWidget(*bitmapWidget))
    {
        success = false;
    }
    else if (false == textCanvas->addWidget(*textWidget))
    {
        success = false;
    }
    else
    {
        bitmapWidget->setName(BMP_WIDGET_NAME);
        bitmapWidget->set(bitmapBuffer, BMP_WIDTH, BMP_HEIGHT);

        textWidget->setName(TEXT_WIDGET_NAME);

        strncpy(lampName, LAMP_WIDGET_NAME, ARRAY_NUM(lampName) - 1);
        lampName[ARRAY_NUM(lampName) - 1] = '\0';
        lampNameNumPos = strlen(lampName);

        for(index = 0u; index < ARRAY_NUM(lampWidgets); ++index)
        {
            if (false == lampCanvas->addWidget(*lampWidgets[index]))
            {
                success = false;
                break;
            }
            else
            {
                lampName[lampNameNumPos] = '\0' + index;
                lampWidgets[index]->setName(lampName);
            }
            
        }
    }

    if (false == success)
    {
        if (NULL != canvas)
        {
            delete canvas;
            canvas = NULL;
        }
        
        if (NULL != bitmapCanvas)
        {
            delete bitmapCanvas;
        }

        if (NULL != bitmapWidget)
        {
            delete bitmapWidget;
        }

        if (NULL != textCanvas)
        {
            delete textCanvas;
        }

        if (NULL != textWidget)
        {
            delete textWidget;
        }

        for(index = 0u; index < ARRAY_NUM(lampWidgets); ++index)
        {
            if (NULL != lampWidgets[index])
            {
                delete lampWidgets[index];
            }
        }
    }

    return success;
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/