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
        MurkaContext& context = m.currentContext;
        auto& c = context; // shorthand of above
        
        bool inside = c.isHovered() * !areInteractiveChildrenHovered(c) * hasMouseFocus(m);
        hovered = inside + draggingNow;
        bool hoveredLocal = hovered/* + externalHovered*/; // this variable is not used outside the widget to avoid feedback loop
        changed = false; // false unless the user changed a value using this knob
        
        if (!enabled) {
            hoveredLocal = false;
        }
        
        int ellipseSize = 5;
        
        std::string displayString = float_to_string(*data, floatingPointPrecision);
        std::string valueText = prefix + displayString + postfix;
        auto font = m.getCurrentFont();
        
        float reticlePositionNorm = (*((float*)dataToControl) - rangeFrom) / (rangeTo - rangeFrom);
        MurkaShape reticlePosition = { c.getSize().x / 2 - 6,
                                      (shape.size.y) * reticlePositionNorm - 6,
                                      12,
                                      12 };
        bool reticleHover = false + draggingNow;
        if (reticlePosition.inside(c.mousePosition)) {
            reticleHover = true;
        }
                
        m.pushStyle();
        m.enableFill();
                
        if (isHorizontal) { // draw horizontally
            m.setColor(133 + 20 * A(reticleHover));
            m.drawLine(ellipseSize / 2, shape.size.y / 2, shape.size.x - ellipseSize, shape.size.y / 2);

            m.setColor(REF_LABEL_TEXT_COLOR);
            //g.setColour(Colour(120, 120, 120));
            m.setFont("Proxima Nova Reg.ttf", fontSize);

            if (movingLabel) {
                m.draw<murka::Label>({reticlePositionNorm * (shape.size.x * 0.8 - ellipseSize) - 16 + shape.size.x * 0.1, 28 - 10, 40, 60}).withAlignment(TEXT_CENTER).text(label).commit();
                m.draw<murka::Label>({reticlePositionNorm * (shape.size.x * 0.8 - ellipseSize) - 16 + shape.size.x * 0.1, 28 + 40, ellipseSize * 6, 20})
                    .withAlignment(TEXT_CENTER).text(valueText).commit();
            } else {
                // TODO: add the value label
                m.draw<murka::Label>({  shape.size.x / 2 - 40, shape.size.y - 20,
                                        80, 20})
                                        .withAlignment(TEXT_CENTER).text(label).commit();
            }
            
            // Draw reticle circle
            if (enabled) {
                m.setColor(M1_ACTION_YELLOW);
            } else {
                m.setColor(DISABLED_PARAM);
            }
            m.drawCircle(reticlePositionNorm * (shape.size.x - ellipseSize), shape.size.y/2 - ellipseSize/2, ellipseSize);
            
        } else { // draw verically
            m.setColor(133 + 20 * A(reticleHover));
            m.drawLine(c.getSize().x / 2, ellipseSize / 2, shape.size.x / 2, shape.size.y - ellipseSize);

            //g.setColour(Colour(120, 120, 120));
            m.setFont("Proxima Nova Reg.ttf", fontSize);
            m.setColor(REF_LABEL_TEXT_COLOR);
            
            if (movingLabel){
                m.draw<murka::Label>({ shape.size.x / 2 - 70, reticlePositionNorm * (shape.size.y - ellipseSize) - 14, 60, 40}).withAlignment(TEXT_CENTER).text(label).commit();
                m.draw<murka::Label>({ shape.size.x / 2 + 10, reticlePositionNorm * (shape.size.y - ellipseSize) - 14,
                                        ellipseSize * 6, 20})
                    .withAlignment(TEXT_CENTER).text(valueText).commit();
            } else {
                m.draw<murka::Label>({  0, shape.size.y / 2 - 10,
                                        shape.size.x / 3, 20})
                                        .withAlignment(TEXT_CENTER).text(label).commit();
                m.draw<murka::Label>({  shape.size.x / 2 - 5, shape.size.y - ellipseSize - 4,
                                        ellipseSize * 6, 20})
                    .withAlignment(TEXT_CENTER).text(valueText).commit();
            }
            
            // Draw reticle circle
            if (enabled) {
                m.setColor(M1_ACTION_YELLOW);
            } else {
                m.setColor(DISABLED_PARAM);
            }
            m.drawCircle(shape.size.x/2 - ellipseSize/2, reticlePositionNorm * (shape.size.y - ellipseSize), ellipseSize);
        }
        m.popStyle();
    
        auto labelPositionY = shape.size.x * 0.8 + 10;
        
        // Action
        
        if ((m.currentContext.mouseDownPressed[0]) && (inside) && (m.currentContext.mousePosition.y < labelPositionY) &&
            (!draggingNow) && (enabled)) {
            draggingNow = true;
            cursorHide();
        }

        if ((draggingNow) && (!m.currentContext.mouseDown[0])) {
            draggingNow = false;
            cursorShow();
        }
        
        // Setting knob value to default if pressed alt while clicking
        bool shouldSetDefault = m.currentContext.isKeyHeld(murka::MurkaKey::MURKA_KEY_ALT) && m.currentContext.mouseDownPressed[0];
        
        // Don't set default by doubleclick if the mouse is in the Label/Text editor zone
        if (m.currentContext.mousePosition.y >= labelPositionY) shouldSetDefault = false;

        if (shouldSetDefault && inside) {
            draggingNow = false;
            *((float*)dataToControl) = defaultValue;
            cursorShow();
            changed = true;
        }
        
        if (draggingNow) {
            if (abs(m.currentContext.mouseDelta.y) >= 1) {
                
                // Shift key fine-tune mode
                float s = speed;  // TODO: check if this speed constant should be dependent on UIScale
                if (m.currentContext.isKeyHeld(murka::MurkaKey::MURKA_KEY_SHIFT)) {
                    s *= 10;
                }
                *((float*)dataToControl) += ( m.currentContext.mouseDelta.y / s) * (rangeTo - rangeFrom);
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
    bool editingTextNow = false;
    bool shouldForceEditorToSelectAll = false;
    
    float* dataToControl = nullptr;
    float defaultValue = 0;
    float speed = 250.;
    
    bool hovered = false;
    bool externalHover = false;
    bool changed = false;
    bool enabled = true;
    bool draggingNow = false;
    bool movingLabel = false;
    bool isHorizontal = false;
    std::string label = "";
    std::string postfix = "º";
    std::string prefix = "";
    double fontSize = 10;
    float rangeFrom = 0;
    float rangeTo = 1;
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
