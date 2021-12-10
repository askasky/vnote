#include "taskhelper.h"

#include <QRegularExpression>
#include <QJsonArray>
#include <QMessageBox>
#include <QPushButton>
#include <QInputDialog>
#include <QProcess>
#include <QDir>

#include <core/vnotex.h>
#include <core/notebookmgr.h>
#include <widgets/mainwindow.h>
#include <widgets/viewarea.h>
#include <widgets/viewwindow.h>
#include <widgets/dialogs/selectdialog.h>
#include <utils/pathutils.h>
#include <task/taskmgr.h>
#include <buffer/buffer.h>
#include <widgets/viewwindow.h>
#include <widgets/viewarea.h>

using namespace vnotex;

const ViewWindow *TaskHelper::getCurrentViewWindow()
{
    return VNoteX::getInst().getMainWindow()->getViewArea()->getCurrentViewWindow();
}

QString TaskHelper::getSelectedText()
{
    auto win = getCurrentViewWindow();
    if (win) {
        return win->selectedText();
    }
    return QString();
}

QStringList TaskHelper::getAllSpecialVariables(const QString &p_name, const QString &p_text)
{
    QStringList vars;
    QRegularExpression re(QString(R"(\$\{[\t ]*%1[\t ]*:[\t ]*(.*?)[\t ]*\})").arg(p_name));
    auto it = re.globalMatch(p_text);
    while (it.hasNext()) {
        vars << it.next().captured(1);
    }
    return vars;
}

QString TaskHelper::replaceAllSepcialVariables(const QString &p_name,
                                               const QString &p_text,
                                               const QMap<QString, QString> &p_map)
{
    auto text = p_text;
    for (auto i = p_map.begin(); i != p_map.end(); i++) {
        auto key = QString(i.key()).replace(".", "\\.").replace("[", "\\[").replace("]", "\\]");
        auto pattern = QRegularExpression(QString(R"(\$\{[\t ]*%1[\t ]*:[\t ]*%2[\t ]*\})").arg(p_name, key));
        text = text.replace(pattern, i.value());
    }
    return text;
}

QString TaskHelper::evaluateJsonExpr(const QJsonObject &p_obj, const QString &p_expr)
{
    QJsonValue value = p_obj;
    for (auto token : p_expr.split('.')) {
        auto pos = token.indexOf('[');
        auto name = token.mid(0, pos);
        value = value.toObject().value(name);
        if (pos == -1) continue;
        if (token.back() == ']') {
            for (auto idx : token.mid(pos+1, token.length()-pos-2).split("][")) {
                bool ok;
                auto index = idx.toInt(&ok);
                if (!ok) throw "Config variable syntax error!";
                value = value.toArray().at(index);
            }
        } else {
            throw "Config variable syntax error!";
        }
    }
    if (value.isBool()) {
        if (value.toBool()) return "true";
        else return "false";
    } else if (value.isDouble()) {
        return QString::number(value.toDouble());
    } else if (value.isNull()) {
        return "null";
    } else if (value.isUndefined()) {
        return "undefined";
    }
    return value.toString();
}
