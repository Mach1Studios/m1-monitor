#pragma once

#include "MurkaTypes.h"
#include "MurkaContext.h"
#include "MurkaView.h"
#include "MurkaInputEventsRegister.h"
#include "MurkaAssets.h"
#include "MurkaLinearLayoutGenerator.h"
#include "MurkaBasicWidgets.h"

using namespace murka;

class M1Slider : public murka::View<M1Slider> {
public:

    void internalDraw(Murka & m) {
        float* data = dataToControl;
        MurkaContext& context = m.currentContext;
        auto& c = context; // shorthand of above
        
        bool inside = context.isHovered() * !areInteractiveChildrenHovered(context) * hasMouseFocus(m);
        bool reticleHover = false + draggingNow;
        
        changed = false;
        hovered = inside + draggingNow; // for external views to be highlighted too if needed
        bool hoveredLocal = hovered; // shouldn't propel hoveredLocal outside so it doesn't feedback

        if (!enabled) {
            hoveredLocal = false;
        }
        
        float reticlePositionNorm = (*((float*)dataToControl) - rangeFrom) / (rangeTo - rangeFrom);
        float inputValueAngleInDegrees = reticlePositionNorm * 360;
        std::string displayString = float_to_string(*data, floatingPointPrecision);
        std::string valueText = prefix + displayString + postfix;
        
        m.pushStyle();
        m.enableFill();
        
        float ellipseSize = 8;
        if (isHorizontal) { // draw horizontally
            m.setColor(133 + 20 * A(reticleHover));
            m.drawLine(ellipseSize / 2, shape.size.y / 2, shape.size.x - ellipseSize, shape.size.y / 2);

            m.setColor(REF_LABEL_TEXT_COLOR);
            //g.setColour(Colour(120, 120, 120));
            m.setFont("Proxima Nova Reg.ttf", fontSize);
            m.draw<murka::Label>({  shape.size.x / 2 - 40, shape.size.y - 20,
                                    80, 20})
                                    .withAlignment(TEXT_CENTER).text(label).commit();
            
            m.draw<murka::Label>({  reticlePositionNorm * (shape.size.x * 0.8 - ellipseSize) - 16 + shape.size.x * 0.1, 4,
                                    40, 20})
                                    .withAlignment(TEXT_CENTER).text(valueText).commit();

            if (movingLabel) {
                m.setColor(REF_LABEL_TEXT_COLOR);
                m.draw<murka::Label>({reticlePositionNorm * (shape.size.x * 0.8 - ellipseSize) - 16 + shape.size.x * 0.1, 28, 40, 60}).withAlignment(TEXT_CENTER).text(label).commit();
            }
            
            // Draw reticle circle
            if (enabled) {
                m.setColor(M1_ACTION_YELLOW);
            } else {
                m.setColor(DISABLED_PARAM);
            }
            m.drawCircle(reticlePositionNorm * (shape.size.x - ellipseSize), shape.size.y/2 - ellipseSize/2, (ellipseSize * A(reticleHover)));
            
        } else { // draw verically
            m.setColor(133 + 20 * A(reticleHover));
            m.drawLine(c.getSize().x / 2, ellipseSize / 2, shape.size.x / 2, shape.size.y - ellipseSize);

            m.setColor(REF_LABEL_TEXT_COLOR);
            //g.setColour(Colour(120, 120, 120));
            m.setFont("Proxima Nova Reg.ttf", fontSize);
            m.draw<murka::Label>({  0, shape.size.y / 2 - 10,
                                    shape.size.x / 3, 20})
                                    .withAlignment(TEXT_CENTER).text(label).commit();
            
            m.draw<murka::Label>({  shape.size.x / 2 - 5, reticlePositionNorm * (shape.size.y - ellipseSize) - 4,
                                    ellipseSize * 6, 20})
                                    .withAlignment(TEXT_CENTER).text(valueText).commit();
            
            if (movingLabel){
                m.setColor(REF_LABEL_TEXT_COLOR);
                m.draw<murka::Label>({(shape.size.x/2) - 51, reticlePositionNorm * (shape.size.y - ellipseSize) - 14, 60, 40}).withAlignment(TEXT_CENTER).text(label).commit();
            }
            
            // Draw reticle circle
            if (enabled) {
                m.setColor(M1_ACTION_YELLOW);
            } else {
                m.setColor(DISABLED_PARAM);
            }
            m.drawCircle(shape.size.x/2 - ellipseSize/2, reticlePositionNorm * (shape.size.y - ellipseSize), (ellipseSize * A(reticleHover)));
        }
        m.popStyle();
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
    bool hovered = false;
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
