#pragma once

#include "../Config.h"
#include "MurkaBasicWidgets.h"
#include "MurkaInputEventsRegister.h"
#include "MurkaView.h"
#include "TextField.h"

using namespace murka;

class M1Slider : public murka::View<M1Slider>
{
public:
    void internalDraw(Murka& m)
    {
        float* data = dataToControl;

        bool isInside = inside() *
                        //!areInteractiveChildrenHovered(c) *
                        //hasMouseFocus(m);
                        (!editingTextNow);

        changed = false; // false unless the user changed a value using this knob
        isHovered = isInside + draggingNow; // for external views to be highlighted too if needed
        bool hoveredLocal = isHovered + externalHover; // shouldn't propel hoveredLocal outside so it doesn't feedback

        if (!enabled)
        {
            hoveredLocal = false;
        }

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

        std::string displayString = float_to_string(*data, floatingPointPrecision);
        std::string valueText = prefix + displayString + postfix;
        auto font = m.getCurrentFont();

        // Parameter reticle position
        int ellipseSize = 5;
        float reticlePositionNorm;
        if (isHorizontal)
        {
            reticlePositionNorm = (*((float*)dataToControl) - rangeFrom) / (rangeTo - rangeFrom);
        }
        else
        {
            reticlePositionNorm = (*((float*)dataToControl) - rangeTo) / (rangeFrom - rangeTo);
        }
        MurkaShape reticlePosition = { getSize().x / 2 - 6,
            (shape.size.y) * reticlePositionNorm - 6,
            12,
            12 };
        bool reticleHover = false + draggingNow;
        if (reticlePosition.inside(mousePosition()))
        {
            reticleHover = true;
        }

        // Orientation Client reticle position
        int indicatorSize = 10;
        float oc_reticlePositionNorm;
        if (isHorizontal)
        {
            oc_reticlePositionNorm = (orientationClientValue - rangeFrom) / (rangeTo - rangeFrom);
        }
        else
        {
            oc_reticlePositionNorm = (orientationClientValue - rangeTo) / (rangeFrom - rangeTo);
        }
        MurkaShape oc_reticlePosition = { getSize().x / 2 - 6,
            (shape.size.y) * reticlePositionNorm - 6,
            12,
            12 };

        m.pushStyle();
        m.enableFill();

        float labelWidth = 40;
        float labelHeight = 30;

        MurkaShape valueTextShape;

        if (isHorizontal)
        { // draw horizontally
            m.setColor(133 + 20 * A(reticleHover));
            m.drawLine(ellipseSize / 2, shape.size.y / 2, shape.size.x - ellipseSize, shape.size.y / 2);

            m.setColor(REF_LABEL_TEXT_COLOR);
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, fontSize);

            if (movingLabel)
            {
                float posX = reticlePositionNorm * (shape.size.x - 2 * ellipseSize) + ellipseSize - labelWidth / 2;
                posX = (std::max)(0.0f, (std::min)(shape.size.x - labelWidth, posX));

                m.prepare<murka::Label>({ posX + 3, shape.size.y / 2 - 35, labelWidth, labelHeight }).withAlignment(TEXT_CENTER).text(label).draw();
                m.prepare<murka::Label>({ posX + 3, shape.size.y / 2 + 22, labelWidth, labelHeight }).withAlignment(TEXT_CENTER).text(valueText).draw();

                valueTextShape = { posX + 3, shape.size.y / 2 + 22, labelWidth, labelHeight };
            }
            else
            {
                m.prepare<murka::Label>({ shape.size.x / 2 - labelWidth / 2, shape.size.y - labelHeight, labelWidth, 30 }).withAlignment(TEXT_CENTER).text(label).draw();
                m.prepare<murka::Label>({ shape.size.x / 2 - labelWidth / 2, shape.size.y + labelHeight, labelWidth, 30 }).withAlignment(TEXT_CENTER).text(valueText).draw();

                valueTextShape = { shape.size.x / 2 - labelWidth / 2, shape.size.y + labelHeight, labelWidth, labelHeight };
            }

            // Draw OC reticle line
            if (orientationClientConnected)
            {
                m.setColor(ORIENTATION_ACTIVE_COLOR);
                m.drawLine(shape.size.x * oc_reticlePositionNorm,
                    shape.size.y / 2 - indicatorSize,
                    shape.size.x * oc_reticlePositionNorm,
                    shape.size.y / 2 + indicatorSize);
            }

            // Draw reticle circle
            if (enabled)
            {
                m.setColor(M1_ACTION_YELLOW);
            }
            else
            {
                m.setColor(DISABLED_PARAM);
            }
            m.drawCircle(reticlePositionNorm * (shape.size.x - 2 * ellipseSize) + ellipseSize, shape.size.y / 2, ellipseSize);
        }
        else
        { // draw verically
            m.setColor(133 + 20 * A(reticleHover));
            m.drawLine(getSize().x / 2, ellipseSize / 2, shape.size.x / 2, shape.size.y - ellipseSize);

            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, fontSize);
            m.setColor(REF_LABEL_TEXT_COLOR);

            if (movingLabel)
            {
                float posY = reticlePositionNorm * (shape.size.y - 2 * ellipseSize) + ellipseSize - m.getCurrentFont()->getLineHeight() / 2;
                posY = (std::max)(0.0f, (std::min)(shape.size.y - m.getCurrentFont()->getLineHeight(), posY));

                m.prepare<murka::Label>({ shape.size.x / 2 - 60, posY, labelWidth, labelHeight }).withAlignment(TEXT_CENTER).text(label).draw();
                m.prepare<murka::Label>({ shape.size.x / 2 + 15, posY, labelWidth, labelHeight }).withAlignment(TEXT_CENTER).text(valueText).draw();

                valueTextShape = { shape.size.x / 2 + 15, posY, labelWidth, labelHeight };
            }
            else
            {
                m.prepare<murka::Label>({ shape.size.x / 2 - 60, shape.size.y / 2 - 9, labelWidth, labelHeight }).withAlignment(TEXT_CENTER).text(label).draw();
                m.prepare<murka::Label>({ shape.size.x / 2 + 15, shape.size.y / 2 - 9, labelWidth, labelHeight }).withAlignment(TEXT_CENTER).text(valueText).draw();

                valueTextShape = { shape.size.x / 2 + 15, shape.size.y / 2 - 9, labelWidth, labelHeight };
            }

            // Draw OC reticle line
            if (orientationClientConnected)
            {
                m.setColor(ORIENTATION_ACTIVE_COLOR);
                m.drawLine(shape.size.x / 2 - indicatorSize,
                    shape.size.y * oc_reticlePositionNorm,
                    shape.size.x / 2 + indicatorSize,
                    shape.size.y * oc_reticlePositionNorm);
            }

            // Draw reticle circle
            if (enabled)
            {
                m.setColor(M1_ACTION_YELLOW);
            }
            else
            {
                m.setColor(DISABLED_PARAM);
            }
            m.drawCircle(shape.size.x / 2, reticlePositionNorm * (shape.size.y - 2 * ellipseSize) + ellipseSize, ellipseSize);
        }
        m.popStyle();

        auto labelPositionY = shape.size.x * 0.8 + 10;
        
        m.setColor(REF_LABEL_TEXT_COLOR);

        if (editingTextNow) {
            auto& textFieldObject =
                m.prepare<TextField>({ valueTextShape.x(), valueTextShape.y() - 5, valueTextShape.width() + 4, valueTextShape.height() - 5})
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
            m.prepare<murka::Label>(valueTextShape)
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

        // Handle double click to edit
        if ((mouseDownPressed(0)) && (!isHovered) && (editingTextNow)) {
            editingTextNow = false;
            deleteTheTextField();
        }

        if (valueTextShape.inside(mousePosition()) && doubleClick() && enabled) {
            editingTextNow = true;
            shouldForceEditorToSelectAll = true;
        }

        // Action

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

        if (shouldSetDefault && inside())
        {
            draggingNow = false;
            *((float*)dataToControl) = defaultValue;
            cursorShow();
            changed = true;
        }

        if (draggingNow)
        {
            // Command / Shift key fine-tune mode
            float s = speed; // TODO: check if this speed constant should be dependent on UIScale
            if (isKeyHeld(murka::MurkaKey::MURKA_KEY_SHIFT) || isKeyHeld(murka::MurkaKey::MURKA_KEY_CONTROL) || isKeyHeld(murka::MurkaKey::MURKA_KEY_COMMAND))
            {
                s *= 50.0f;
            }

            if (isHorizontal)
            {
                *((float*)dataToControl) -= (mouseDelta().x / s) * (rangeTo - rangeFrom);
            }
            else
            {
                *((float*)dataToControl) -= (mouseDelta().y / s) * (rangeFrom - rangeTo);
            }

            if (*((float*)dataToControl) > rangeTo)
            {
                *((float*)dataToControl) = rangeTo;
            }

            if (*((float*)dataToControl) < rangeFrom)
            {
                *((float*)dataToControl) = rangeFrom;
            }
            changed = true;
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

    bool draggingNow = false;
    bool editingTextNow = false;
    bool shouldForceEditorToSelectAll = false;

    float rangeFrom = 0;
    float rangeTo = 1;

    std::string postfix = "";
    std::string prefix = "";

    float* dataToControl = nullptr;
    float defaultValue = 0;
    float speed = 250.;
    bool isHovered = false;
    bool externalHover = false;
    bool changed = false;
    bool enabled = true;
    bool movingLabel = false;
    bool isHorizontal = false;
    std::string label = "";
    double fontSize = 10;
    int floatingPointPrecision = 1;

    bool orientationClientConnected = false;
    float orientationClientValue = 0;

    std::function<void()> cursorHide, cursorShow;

    M1Slider& controlling(float* dataPointer)
    {
        dataToControl = dataPointer;
        return *this;
    }

    M1Slider& hasMovingLabel(bool movingLabel_)
    {
        movingLabel = movingLabel_;
        return *this;
    }

    M1Slider& drawHorizontal(bool isHorizontal_)
    {
        isHorizontal = isHorizontal_;
        return *this;
    }

    M1Slider& withLabel(std::string label_)
    {
        label = label_;
        return *this;
    }

    M1Slider& withFontSize(double fontSize_)
    {
        fontSize = fontSize_;
        return *this;
    }
};
