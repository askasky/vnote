#ifndef TASKHELPER_H
#define TASKHELPER_H

#include <QString>
#include <QSharedPointer>

class QProcess;

namespace vnotex
{
    class Notebook;
    class Task;
    class Buffer;
    class ViewWindow;

    class TaskHelper
    {
    public:
        TaskHelper() = delete;

        static Buffer *getCurrentBuffer();

        static QString getSelectedText();

        static QStringList getAllSpecialVariables(const QString &p_name, const QString &p_text);

        static QString replaceAllSepcialVariables(const QString &p_name,
                                                  const QString &p_text,
                                                  const QMap<QString, QString> &p_map);

        static QString evaluateJsonExpr(const QJsonObject &p_obj,
                                        const QString &p_expr);

    private:
        static const ViewWindow *getCurrentViewWindow();
    };
} // ns vnotex

#endif // TASKHELPER_H
