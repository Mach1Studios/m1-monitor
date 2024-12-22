#pragma once

#include "../Config.h"
#include "MurkaBasicWidgets.h"
#include "MurkaInputEventsRegister.h"
#include "TextField.h"

using namespace murka;

class M1Radial : public murka::View<M1Radial>
{
public:
    void internalDraw(Murka& m)
    {
        float* data = dataToControl;
        float inputValueNormalised = ((*data - rangeFrom) / (rangeTo - rangeFrom));
        inputValueNormalised -= 0.5f; // rescale so that the median value is upward

        bool isInside = inside() * (!editingTextNow);

        changed = false;
        isHovered = isInside + draggingNow; // for external views to be highlighted too if needed
        bool hoveredLocal = isHovered + externalHover; // shouldn't propel hoveredLocal outside so it doesn't feedback

        if (!enabled)
        {
            hoveredLocal = false;
        }

        std::string displayString = float_to_string(*data, floatingPointPrecision);
        std::string valueText = prefix + displayString + postfix;
        auto font = m.getCurrentFont();

        m.pushStyle();

        // Increase circle resolution
        m.setCircleResolution(64);

        m.setLineWidth(2);
        m.enableFill();

        m.setColor(GRID_LINES_1_RGBA);
        m.drawCircle(shape.size.x / 2, shape.size.y / 2, shape.size.x / 2);

        m.setColor(REF_LABEL_TEXT_COLOR);
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, fontSize);
        m.prepare<murka::Label>({ (shape.size.x / 2 - 20), shape.size.y / 4, 40, 30 }).withAlignment(TEXT_CENTER).text(label).draw();

        m.setColor(ENABLED_PARAM);

        // Circular lines
        juce::Point<float> center = juce::Point<float>(shape.size.x / 2, shape.size.y / 2);

        for (int i = 0; i < 8 * 4; i++)
        {
            float angle = (juce::MathConstants<float>::twoPi / (8.0f * 4)) * i;
            float startL = shape.size.x / 2 - 12;
            float endL = shape.size.x / 2 - 5;
            juce::Point<float> start = center + juce::Point<float>(cos(angle) * startL, sin(angle) * startL);
            juce::Point<float> end = center + juce::Point<float>(cos(angle) * endL, sin(angle) * endL);
            m.drawLine(start.x, start.y, end.x, end.y);
        }

        for (int i = 0; i < 8; i++)
        {
            float angle = (juce::MathConstants<float>::twoPi / 8.0f) * i;
            float startL = shape.size.x / 2 - 24;
            float endL = shape.size.x / 2 - 5;
            juce::Point<float> start = center + juce::Point<float>(cos(angle) * startL, sin(angle) * startL);
            juce::Point<float> end = center + juce::Point<float>(cos(angle) * endL, sin(angle) * endL);
            m.drawLine(start.x, start.y, end.x, end.y);
        }

        // Draw OC line
        if (orientationClientConnected)
        {
            m.setLineWidth(6);
            m.setColor(ORIENTATION_ACTIVE_COLOR);
            float oc_valueNorm = ((orientationClientValue - rangeFrom) / (rangeTo - rangeFrom));
            oc_valueNorm -= 0.5f; // rescale so that the median value is upward
            float oc_angle = juce::MathConstants<float>::twoPi * (oc_valueNorm - 0.25f);
            juce::Point<float> centralLineStart = center + juce::Point<float>(cos(oc_angle) * 75, sin(oc_angle) * 75);
            juce::Point<float> centralLineEnd = center + juce::Point<float>(cos(oc_angle) * (shape.size.x / 2 - 7), sin(oc_angle) * (shape.size.x / 2 - 7));
            // draw main line
            m.drawLine(centralLineStart.x, centralLineStart.y, centralLineEnd.x, centralLineEnd.y);
            m.setLineWidth(2);
        }

        // The main line
        if (enabled)
        {
            m.setColor(M1_ACTION_YELLOW);
        }
        else
        {
            m.setColor(DISABLED_PARAM);
        }

        float angle = juce::MathConstants<float>::twoPi * (inputValueNormalised - 0.25f);
        float angleSize = juce::MathConstants<float>::halfPi;

        int numsteps = 60;
        for (int i = 0; i < numsteps; i++)
        {
            float phase0 = (float)i / (float)numsteps;
            float phase1 = ((float)i + 1) / (float)numsteps;

            float angle0 = phase0 * angleSize + angle - angleSize * 0.5f;
            float angle1 = phase1 * angleSize + angle - angleSize * 0.5f;

            juce::Point<float> lineStart = center + juce::Point<float>(cos(angle0 - 0.01f) * (shape.size.x / 2 - 1), sin(angle0 - 0.01f) * (shape.size.y / 2 - 1));
            juce::Point<float> lineEnd = center + juce::Point<float>(cos(angle1 + 0.01f) * (shape.size.x / 2 - 1), sin(angle1 + 0.01f) * (shape.size.y / 2 - 1));
            //draw arc
            m.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y);
        }

        juce::Point<float> centralLineStart = center + juce::Point<float>(cos(angle) * 25, sin(angle) * 25);
        juce::Point<float> centralLineEnd = center + juce::Point<float>(cos(angle) * (shape.size.x / 2 - 7), sin(angle) * (shape.size.x / 2 - 7));
        // draw main line
        m.drawLine(centralLineStart.x, centralLineStart.y, centralLineEnd.x, centralLineEnd.y);

        std::function<void()> deleteTheTextField = [&]() {
            // Temporary solution to delete the TextField:
            // Searching for an id to delete the text field widget.
            // To be redone after the UI library refactoring.

            imIdentifier idToDelete;
            for (auto childTuple : imChildren)
            {
                auto childIdTuple = childTuple.first;
                if (std::get<1>(childIdTuple) == typeid(TextField).name())
                {
                    idToDelete = childIdTuple;
                }
            }
            imChildren.erase(idToDelete);
        };

        // value label
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, fontSize);
        juceFontStash::Rectangle label_box = m.getCurrentFont()->getStringBoundingBox(valueText, 0, 0);

        MurkaShape valueTextShape = {
            shape.size.x / 2 - label_box.width / 2,
            shape.size.y / 2 - label_box.height / 2,
            label_box.width + 10,
            label_box.height
        };

        if (editingTextNow) {
            auto& textFieldObject =
                m.prepare<TextField>({ valueTextShape.x() - 5, valueTextShape.y() - 5, valueTextShape.width() + 10, valueTextShape.height() + 10 })
                    .controlling(data)
                    .withPrecision(floatingPointPrecision)
                    .forcingEditorToSelectAll(shouldForceEditorToSelectAll)
                    .onlyAllowNumbers(true)
                    .grabKeyboardFocus()
                    .draw();

            auto textFieldEditingFinished = textFieldObject.editingFinished;
            
            if (shouldForceEditorToSelectAll) {
                shouldForceEditorToSelectAll = false;
            }

            if (!textFieldEditingFinished) {
                textFieldObject.activated = true;
                claimKeyboardFocus();
            }
            
            if (textFieldEditingFinished) {
                editingTextNow = false;
                changed = true;
                deleteTheTextField();
            }
        } else {
            m.prepare<murka::Label>({ valueTextShape.x(), valueTextShape.y(), valueTextShape.width(), valueTextShape.height() })
                .withAlignment(TEXT_CENTER)
                .text(valueText)
                .draw();
        }
        
        bool hoveredValueText = false;
        if (valueTextShape.inside(mousePosition()) && !editingTextNow && enabled)
        {
            m.drawRectangle(valueTextShape.x(),
                valueTextShape.y(),
                2,
                2);
            m.drawRectangle(valueTextShape.x() + valueTextShape.width() + 2,
                valueTextShape.y(),
                2,
                2);
            m.drawRectangle(valueTextShape.x(),
                valueTextShape.y() + 12,
                2,
                2);
            m.drawRectangle(valueTextShape.x() + valueTextShape.width() + 2,
                valueTextShape.y() + 12,
                2,
                2);
            hoveredValueText = true;
        }
        
        m.popStyle();

        m.setColor(100 + 110 * enabled + A(30 * hoveredLocal), 255);
        auto labelPositionY = shape.size.x * 0.8 + 10;

        // Action

        if ((mouseDownPressed(0)) && (!isHovered) && (editingTextNow))
        {
            // Pressed outside the knob widget while editing text. Aborting the text edit
            editingTextNow = false;
            deleteTheTextField();
        }

        if (valueTextShape.inside(mousePosition()) && doubleClick() && enabled) {
            editingTextNow = true;
            shouldForceEditorToSelectAll = true;
        }

        if ((mouseDownPressed(0)) && (inside()) && (mousePosition().y < labelPositionY) && (!draggingNow) && (enabled))
        {
            draggingNow = true;
            cursorHide();
        }

        if ((draggingNow) && (!mouseDown(0)))
        {
            draggingNow = false;
            cursorShow();
        }

        // Setting knob value to default if pressed alt while clicking
        bool shouldSetDefault = isKeyHeld(murka::MurkaKey::MURKA_KEY_ALT) && mouseDownPressed(0);

        // Don't set default by doubleclick if the mouse is in the Label/Text editor zone
        if (mousePosition().y >= labelPositionY)
            shouldSetDefault = false;

        if (shouldSetDefault && isInside)
        {
            draggingNow = false;
            *((float*)dataToControl) = defaultValue;
            cursorShow();
            changed = true;
        }

        if (draggingNow)
        {
            if (abs(mouseDelta().y) >= 1)
            {
                // Command / Shift key fine-tune mode
                float s = speed; // TODO: check if this speed constant should be dependent on UIScale
                if (isKeyHeld(murka::MurkaKey::MURKA_KEY_SHIFT) || isKeyHeld(murka::MurkaKey::MURKA_KEY_CONTROL) || isKeyHeld(murka::MurkaKey::MURKA_KEY_COMMAND))
                {
                    s *= 50.0f;
                }
                *((float*)dataToControl) += (mouseDelta().y / s) * (rangeTo - rangeFrom);
            }

            // endless behavior
            if (*((float*)dataToControl) > rangeTo)
            {
                *((float*)dataToControl) -= (rangeTo - rangeFrom);
            }

            if (*((float*)dataToControl) < rangeFrom)
            {
                *((float*)dataToControl) += (rangeTo - rangeFrom);
            }
            changed = true;
        }

        // hot key resets
        if (activated)
        {
            if (isKeyReleased(murka::MurkaKey::MURKA_KEY_UP))
            {
                *((float*)dataToControl) = 0.0f;
                changed = true;
            }
            if (isKeyReleased(murka::MurkaKey::MURKA_KEY_RIGHT))
            {
                *((float*)dataToControl) = 90.0f;
                changed = true;
            }
            if (isKeyReleased(murka::MurkaKey::MURKA_KEY_DOWN))
            {
                *((float*)dataToControl) = 180.0f;
                changed = true;
            }
            if (isKeyReleased(murka::MurkaKey::MURKA_KEY_LEFT))
            {
                *((float*)dataToControl) = 270.0f;
                changed = true;
            }
        }
    }

    // Text based declares
    std::stringstream converterStringStream;
    std::string float_to_string(float input, int precision)
    {
        converterStringStream.str(std::string());
        converterStringStream << std::fixed << std::setprecision(precision) << input;
        return (converterStringStream.str());
    }
    bool editingTextNow = false;
    bool shouldForceEditorToSelectAll = false;

    float* dataToControl = nullptr;
    float defaultValue = 0;
    float speed = 250.;

    bool activated = false;

    bool isHovered = false;
    bool externalHover = false;
    bool changed = false;
    bool enabled = true;
    bool draggingNow = false;
    std::string label = "";
    std::string postfix = "";
    std::string prefix = "";
    double fontSize = 9;
    float rangeFrom = 0;
    float rangeTo = 1;
    int floatingPointPrecision = 1;

    bool orientationClientConnected = false;
    float orientationClientValue = 0;

    std::function<void()> cursorHide, cursorShow;

    M1Radial& controlling(float* dataPointer)
    {
        dataToControl = dataPointer;
        return *this;
    }

    M1Radial& withLabel(std::string label_)
    {
        label = label_;
        return *this;
    }

    M1Radial& withFontSize(double fontSize_)
    {
        fontSize = fontSize_;
        return *this;
    }
};
