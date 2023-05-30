#pragma once

#include "MurkaView.h"
#include "MurkaInputEventsRegister.h"
#include "MurkaBasicWidgets.h"
#include "TextField.h"
#include "../Config.h"

using namespace murka;

class M1Slider : public murka::View<M1Slider> {
public:

    void internalDraw(Murka & m) {
        float* data = dataToControl;
        
        bool isInside = inside() *
        //!areInteractiveChildrenHovered(c) *
        //hasMouseFocus(m);
        (!editingTextNow);

        changed = false; // false unless the user changed a value using this knob
        isHovered = isInside + draggingNow; // for external views to be highlighted too if needed
        bool hoveredLocal = isHovered + externalHover; // shouldn't propel hoveredLocal outside so it doesn't feedback

        if (!enabled) {
            hoveredLocal = false;
        }

        std::function<void()> deleteTheTextField = [&]() {
            // Temporary solution to delete the TextField:
            // Searching for an id to delete the text field widget.
            // To be redone after the UI library refactoring.
            
            imIdentifier idToDelete;
            for (auto childTuple: imChildren) {
                auto childIdTuple = childTuple.first;
                if (std::get<1>(childIdTuple) == typeid(TextField).name()) {
                    idToDelete = childIdTuple;
                }
            }
            imChildren.erase(idToDelete);
        };
        
        std::string displayString = float_to_string(*data, floatingPointPrecision);
        std::string valueText = prefix + displayString + postfix;
        auto font = m.getCurrentFont();
        
        int ellipseSize = 5;
        float reticlePositionNorm = (*((float*)dataToControl) - rangeFrom) / (rangeTo - rangeFrom);
        MurkaShape reticlePosition = { getSize().x / 2 - 6,
                                      (shape.size.y) * reticlePositionNorm - 6,
                                      12,
                                      12 };
        bool reticleHover = false + draggingNow;
        if (reticlePosition.inside(mousePosition())) {
            reticleHover = true;
        }
                
        m.pushStyle();
        m.enableFill();
                
        if (isHorizontal) { // draw horizontally
            m.setColor(133 + 20 * A(reticleHover));
            m.drawLine(ellipseSize / 2, shape.size.y / 2, shape.size.x - ellipseSize, shape.size.y / 2);

            m.setColor(REF_LABEL_TEXT_COLOR);
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, fontSize);

            if (movingLabel) {
                m.prepare<murka::Label>({reticlePositionNorm * (shape.size.x * 0.8 - ellipseSize) - 16 + shape.size.x * 0.1, 28 - 10, 40, 60}).withAlignment(TEXT_CENTER).text(label).draw();
                m.prepare<murka::Label>({reticlePositionNorm * (shape.size.x * 0.8 - ellipseSize) - 16 + shape.size.x * 0.1, 28 + 40, ellipseSize * 6, 20})
                    .withAlignment(TEXT_CENTER).text(valueText).draw();
            } else {
                // TODO: add the value label
                m.prepare<murka::Label>({  shape.size.x / 2 - 40, shape.size.y - 20,
                                        80, 20})
                                        .withAlignment(TEXT_CENTER).text(label).draw();
            }
            
            // Draw reticle circle
            if (enabled) {
                m.setColor(M1_ACTION_YELLOW);
            } else {
                m.setColor(DISABLED_PARAM);
            }
            m.drawCircle(reticlePositionNorm * (shape.size.x - ellipseSize), shape.size.y/2 , ellipseSize);
            
        } else { // draw verically
            m.setColor(133 + 20 * A(reticleHover));
            m.drawLine(getSize().x / 2, ellipseSize / 2, shape.size.x / 2, shape.size.y - ellipseSize);

            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, fontSize);
            m.setColor(REF_LABEL_TEXT_COLOR);
            
            if (movingLabel){
                m.prepare<murka::Label>({ shape.size.x / 2 - 70, reticlePositionNorm * (shape.size.y - ellipseSize) - 14, 60, 40}).withAlignment(TEXT_CENTER).text(label).draw();
                m.prepare<murka::Label>({ shape.size.x / 2 + 10,
                    reticlePositionNorm * (shape.size.y - ellipseSize) - 9,
                                        ellipseSize * 6, 20})
                    .withAlignment(TEXT_CENTER).text(valueText).draw();
            } else {
                m.prepare<murka::Label>({  0, shape.size.y / 2 - 10,
                                        shape.size.x / 3, 20})
                                        .withAlignment(TEXT_CENTER).text(label).draw();
                m.prepare<murka::Label>({  shape.size.x / 2 - 5, shape.size.y - ellipseSize - 4,
                                        ellipseSize * 6, 20})
                    .withAlignment(TEXT_CENTER).text(valueText).draw();
            }
            
            // Draw reticle circle
            if (enabled) {
                m.setColor(M1_ACTION_YELLOW);
            } else {
                m.setColor(DISABLED_PARAM);
            }
            m.drawCircle(shape.size.x/2, reticlePositionNorm * (shape.size.y - ellipseSize), ellipseSize);
        }
        m.popStyle();
    
        auto labelPositionY = shape.size.x * 0.8 + 10;
        
        // Action
        
        if ((mouseDownPressed(0)) && (inside()) && (mousePosition().y < labelPositionY) &&
            (!draggingNow) && (enabled)) {
            draggingNow = true;
            cursorHide();
        }

        if ((draggingNow) && (!mouseDown(0))) {
            draggingNow = false;
            cursorShow();
        }
        
        // Setting knob value to default if pressed alt while clicking
        bool shouldSetDefault = isKeyHeld(murka::MurkaKey::MURKA_KEY_ALT) && mouseDownPressed(0);
        
        // Don't set default by doubleclick if the mouse is in the Label/Text editor zone
        if (mousePosition().y >= labelPositionY) shouldSetDefault = false;

        if (shouldSetDefault && inside()) {
            draggingNow = false;
            *((float*)dataToControl) = defaultValue;
            cursorShow();
            changed = true;
        }
        
        if (draggingNow) {
                
                // Shift key fine-tune mode
                float s = speed;  // TODO: check if this speed constant should be dependent on UIScale
                if (isKeyHeld(murka::MurkaKey::MURKA_KEY_SHIFT)) {
                    s *= 10;
                }
                
                if (isHorizontal) {
                        *((float*)dataToControl) -= ( mouseDelta().x / s) * (rangeTo - rangeFrom);
                } else {
                        *((float*)dataToControl) -= ( mouseDelta().y / s) * (rangeTo - rangeFrom);
                }
            
            if (*((float*)dataToControl) > rangeTo) {
                *((float*)dataToControl) = rangeTo;
            }
            
            if (*((float*)dataToControl) < rangeFrom) {
                *((float*)dataToControl) = rangeFrom;
            }
            changed = true;
        }
    }
    
    // Text based declares
    std::stringstream converterStringStream;
    std::string float_to_string(float input, int precision) {
        converterStringStream.str(std::string());
        converterStringStream << std::fixed << std::setprecision(precision) << input;
        return (converterStringStream.str());
    }
    
    bool draggingNow = false;
    bool editingTextNow = false;
    bool shouldForceEditorToSelectAll = false;
    
    float rangeFrom = 0;
    float rangeTo = 1;

    std::string postfix = "º";
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

    std::function<void()> cursorHide, cursorShow;
    
    M1Slider & controlling(float* dataPointer) {
        dataToControl = dataPointer;
        return *this;
    }
    
    M1Slider & hasMovingLabel(bool movingLabel_) {
        movingLabel = movingLabel_;
        return *this;
    }
    
    M1Slider & drawHorizontal(bool isHorizontal_) {
        isHorizontal = isHorizontal_;
        return *this;
    }
    
    M1Slider & withLabel(std::string label_) {
        label = label_;
        return *this;
    }
    
    M1Slider & withFontSize(double fontSize_) {
        fontSize = fontSize_;
        return *this;
    }
};
