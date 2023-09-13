#pragma once

#include "MurkaBasicWidgets.h"
#include "TextField.h"
#include "../Config.h"

using namespace murka;

class M1Radial : public murka::View<M1Radial> {
public:
    void internalDraw(Murka & m) {
        float* data = dataToControl;
        float inputValueNormalised = ((*data - rangeFrom) / (rangeTo - rangeFrom));

        bool isInside = inside()
            * (!editingTextNow);
        
        changed = false;
        isHovered = isInside + draggingNow; // for external views to be highlighted too if needed
        bool hoveredLocal = isHovered + externalHover; // shouldn't propel hoveredLocal outside so it doesn't feedback

        if (!enabled) {
            hoveredLocal = false;
        }
        
        std::string displayString = float_to_string(*data, floatingPointPrecision);
        std::string valueText = prefix + displayString + postfix;
        auto font = m.getCurrentFont();
        auto valueTextBbox = font->getStringBoundingBox(valueText, 0, 0);
                
        m.pushStyle();
        
        // Increase circle resolution
        m.setCircleResolution(64);
        
        m.setLineWidth(2);
        m.enableFill();
        
        m.setColor(GRID_LINES_1_RGBA);
        m.drawCircle(shape.size.x/2, shape.size.y/2, shape.size.x/2);
        
        m.setColor(REF_LABEL_TEXT_COLOR);
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, fontSize);
        m.prepare<murka::Label>({(shape.size.x / 2 - 20), shape.size.y / 4, 40, 30}).withAlignment(TEXT_CENTER).text(label).draw();
        
        m.setColor(ENABLED_PARAM);
        
        // Circular lines
        juce::Point<float> center = juce::Point<float>(shape.size.x / 2, shape.size.y / 2);

        for (int i = 0; i < 8 * 4; i++) {
            float angle = ((3.14 * 2.) / (8. * 4)) * i;
            float startL = shape.size.x / 2 - 12;
            float endL = shape.size.x / 2 - 5;
            juce::Point<float> start = center +
            juce::Point<float>(cos(angle) * startL, sin(angle) * startL);
            juce::Point<float> end = center +
            juce::Point<float>(cos(angle) * endL, sin(angle) * endL);
            m.drawLine(start.x, start.y, end.x, end.y);
        }

        for (int i = 0; i < 8; i++) {
            float angle = ((3.14 * 2.) / 8.) * i;
            float startL = shape.size.x / 2 - 24;
            float endL = shape.size.x / 2 - 5;
            juce::Point<float> start = center +
                                 juce::Point<float>(cos(angle) * startL, sin(angle) * startL);
            juce::Point<float> end = center +
                                 juce::Point<float>(cos(angle) * endL, sin(angle) * endL);
            m.drawLine(start.x, start.y, end.x, end.y);
        }
        
        // The main line
        if (enabled) {
            m.setColor(M1_ACTION_YELLOW);
        } else {
            m.setColor(DISABLED_PARAM);
        }
        
        float angle = 3.14 * 2 * (inputValueNormalised - 0.25);
        float angleSize = 3.14 * 0.5;
        
        int numsteps = 60;
        for (int i = 0; i < numsteps; i++) {
            float phase0 = (float)i / (float)numsteps;
            float phase1 = ((float)i + 1) / (float)numsteps;
            
            float angle0 = phase0 * angleSize + angle - angleSize * 0.5;
            float angle1 = phase1 * angleSize + angle - angleSize * 0.5;

            juce::Point<float> lineStart = center +
                                    juce::Point<float>(cos(angle0 - 0.01) * (shape.size.x / 2 - 1), sin(angle0 - 0.01) * (shape.size.y / 2 - 1));
            juce::Point<float> lineEnd = center +
                                    juce::Point<float>(cos(angle1 + 0.01) * (shape.size.x / 2 - 1), sin(angle1 + 0.01) * (shape.size.y / 2 - 1));
            //draw arc
            m.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y);
        }
        
        // TODO: have start point drawn a little further from center to make more room for value label
        juce::Point<float> centralLineStart = center + juce::Point<float>(cos(angle) * 15,
                                                              sin(angle) * 15) ;
        juce::Point<float> centralLineEnd = center + juce::Point<float>(cos(angle) * (shape.size.x / 2 - 7), sin(angle) * (shape.size.x / 2 - 7));
        // draw main line
        m.drawLine(centralLineStart.x, centralLineStart.y, centralLineEnd.x, centralLineEnd.y);
        
        // value label
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, fontSize);
        m.prepare<murka::Label>({(shape.size.x / 2 - 20), shape.size.y / 2 - 10, 40, 40}).withAlignment(TEXT_CENTER).text(valueText).draw();
        
        m.popStyle();
        
        m.setColor(100 + 110 * enabled + A(30 * hoveredLocal), 255);
        auto labelPositionY = shape.size.x * 0.8 + 10;
        
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

        MurkaShape valueTextShape = { shape.size.x / 2 - valueTextBbox.width / 2 - 5,
                                     shape.size.x * 0.8 + 10,
                                     valueTextBbox.width + 10,
                                     valueTextBbox.height };
        
        bool hoveredValueText = false;
        if (valueTextShape.inside(mousePosition()) && !editingTextNow && enabled) {
            m.drawRectangle(valueTextShape.x() - 2,
                             valueTextShape.y(),
                             2,
                             2);
            m.drawRectangle(valueTextShape.x() + valueTextShape.width() + 2,
                             valueTextShape.y(),
                             2,
                             2);
            m.drawRectangle(valueTextShape.x() - 2,
                             valueTextShape.y() + valueTextShape.height(),
                             2,
                             2);
            m.drawRectangle(valueTextShape.x() + valueTextShape.width() + 2,
                             valueTextShape.y() + valueTextShape.height(),
                             2,
                             2);
            hoveredValueText = true;
        }
        
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

        if (shouldSetDefault && isInside) {
            draggingNow = false;
            *((float*)dataToControl) = defaultValue;
            cursorShow();
            changed = true;
        }
        
        if (draggingNow) {
            if (abs(mouseDelta().y) >= 1) {
                
                // Shift key fine-tune mode
                float s = speed;  // TODO: check if this speed constant should be dependent on UIScale
                if (isKeyHeld(murka::MurkaKey::MURKA_KEY_SHIFT)) {
                    s *= 10;
                }
                *((float*)dataToControl) += ( mouseDelta().y / s) * (rangeTo - rangeFrom);
            }
            
            // endless behavior
            if (*((float*)dataToControl) > rangeTo) {
                *((float*)dataToControl) -= (rangeTo - rangeFrom);
            }
            
            if (*((float*)dataToControl) < rangeFrom) {
                *((float*)dataToControl) += (rangeTo - rangeFrom);
            }
            changed = true;
        }
        
        // hot key resets
        if (isKeyReleased(murka::MurkaKey::MURKA_KEY_UP)) {
            *((float*)dataToControl) = 0.0f;
            changed = true;
        }
        if (isKeyReleased(murka::MurkaKey::MURKA_KEY_RIGHT)) {
            *((float*)dataToControl) = 90.0f;
            changed = true;
        }
        if (isKeyReleased(murka::MurkaKey::MURKA_KEY_DOWN)) {
            *((float*)dataToControl) = 180.0f;
            changed = true;
        }
        if (isKeyReleased(murka::MurkaKey::MURKA_KEY_LEFT)) {
            *((float*)dataToControl) = 270.0f;
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

    std::function<void()> cursorHide, cursorShow;
    
    M1Radial & controlling(float* dataPointer) {
        dataToControl = dataPointer;
        return *this;
    }
    
    M1Radial & withLabel(std::string label_) {
        label = label_;
        return *this;
    }
    
    M1Radial & withFontSize(double fontSize_) {
        fontSize = fontSize_;
        return *this;
    }
};
