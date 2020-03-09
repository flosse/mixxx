#include <QScriptValueIterator>

#include "controllers/colormapperjsproxy.h"

ColorMapperJSProxy::ColorMapperJSProxy(QScriptEngine* pScriptEngine, QMap<QRgb, QVariant> availableColors)
        : m_pScriptEngine(pScriptEngine),
          m_colorMapper(new ColorMapper(availableColors)) {
}

QScriptValue ColorMapperJSProxy::getNearestColor(uint colorCode) {
    auto result = m_colorMapper->getNearestColor(static_cast<QRgb>(colorCode));
    QScriptValue jsColor = m_pScriptEngine->newObject();
    jsColor.setProperty("red", qRed(result.first));
    jsColor.setProperty("green", qGreen(result.first));
    jsColor.setProperty("blue", qBlue(result.first));
    return jsColor;
}

QScriptValue ColorMapperJSProxy::getNearestValue(uint colorCode) {
    QPair<QRgb, QVariant> result = m_colorMapper->getNearestColor(static_cast<QRgb>(colorCode));
    return m_pScriptEngine->toScriptValue(result.second);
}

QScriptValue ColorMapperJSProxyConstructor(QScriptContext* pScriptContext, QScriptEngine* pScriptEngine) {
    QMap<QRgb, QVariant> availableColors;
    if (pScriptContext->argumentCount() != 1) {
        pScriptContext->throwError(
                QStringLiteral("Failed to create ColorMapper object: constructor takes exactly one argument!"));
        return pScriptEngine->undefinedValue();
    }
    QScriptValue argument = pScriptContext->argument(0);
    if (!argument.isValid() || !argument.isObject()) {
        pScriptContext->throwError(
                QStringLiteral("Failed to create ColorMapper object: argument needs to be an object!"));
        return pScriptEngine->undefinedValue();
    }

    QScriptValueIterator it(argument);
    while (it.hasNext()) {
        it.next();
        QColor color(it.name());
        if (color.isValid()) {
            availableColors.insert(color.rgb(), it.value().toVariant());
        } else {
            pScriptContext->throwError(
                    QStringLiteral("Invalid color name passed to ColorMapper: ") + it.name());
            continue;
        }
    }

    if (availableColors.isEmpty()) {
        pScriptContext->throwError(
                QStringLiteral("Failed to create ColorMapper object: available colors mustn't be empty!"));
        return pScriptEngine->undefinedValue();
    }

    QObject* colorMapper = new ColorMapperJSProxy(pScriptEngine, availableColors);
    return pScriptEngine->newQObject(colorMapper, QScriptEngine::ScriptOwnership);
}
