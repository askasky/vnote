#include "taskvariablemgr.h"

#include <QRegularExpression>
#include <QInputDialog>
#include <QApplication>
#include <QRandomGenerator>
#include <QTimeZone>
#include <QProcess>

#include <core/vnotex.h>
#include <core/notebookmgr.h>
#include <core/configmgr.h>
#include <core/mainconfig.h>
#include <widgets/mainwindow.h>
#include <widgets/dialogs/selectdialog.h>
#include <notebook/notebook.h>
#include <notebook/node.h>
#include <utils/pathutils.h>
#include <buffer/buffer.h>
#include <widgets/viewwindow.h>
#include <widgets/viewarea.h>

#include "task.h"
#include "taskhelper.h"
#include "taskmgr.h"
#include "shellexecution.h"

using namespace vnotex;


TaskVariable::TaskVariable(const QString &p_name, const Func &p_func)
    : m_name(p_name),
      m_func(p_func)
{
}

QString TaskVariable::evaluate(const Task *p_task, const QString &p_value) const
{
    return m_func(p_task, p_value);
}


const QString TaskVariableMgr::c_variableSymbolRegExp = QString(R"(\$\{([^${}:]+)(?::([^${}:]+))?\})");

TaskVariableMgr::TaskVariableMgr(TaskMgr *p_taskMgr)
    : m_taskMgr(p_taskMgr)
{
}

void TaskVariableMgr::init()
{
    initVariables();
}

void TaskVariableMgr::initVariables()
{
    m_variables.clear();

    initNotebookVariables();

    initBufferVariables();

    initTaskVariables();
}

void TaskVariableMgr::initNotebookVariables()
{
    addVariable("notebookFolder", [](const Task *, const QString &) {
        auto notebook = TaskVariableMgr::getCurrentNotebook();
        if (notebook) {
            return PathUtils::cleanPath(notebook->getRootFolderAbsolutePath());
        } else {
            return QString();
        }
    });
    addVariable("notebookFolderName", [](const Task *, const QString &) {
        auto notebook = TaskVariableMgr::getCurrentNotebook();
        if (notebook) {
            return PathUtils::dirName(notebook->getRootFolderPath());
        } else {
            return QString();
        }
    });
    addVariable("notebookName", [](const Task *, const QString &) {
        auto notebook = TaskVariableMgr::getCurrentNotebook();
        if (notebook) {
            return notebook->getName();
        } else {
            return QString();
        }
    });
    addVariable("notebookDescription", [](const Task *, const QString &) {
        auto notebook = TaskVariableMgr::getCurrentNotebook();
        if (notebook) {
            return notebook->getDescription();
        } else {
            return QString();
        }
    });
}

void TaskVariableMgr::initBufferVariables()
{
    addVariable("buffer", [](const Task *, const QString &) {
        auto buffer = getCurrentBuffer();
        if (buffer) {
            return PathUtils::cleanPath(buffer->getPath());
        }
        return QString();
    });
    addVariable("bufferNotebookFolder", [](const Task *, const QString &) {
        auto buffer = getCurrentBuffer();
        if (buffer) {
            auto node = buffer->getNode();
            if (node) {
                return PathUtils::cleanPath(node->getNotebook()->getRootFolderAbsolutePath());
            }
        }
        return QString();
    });
    addVariable("bufferRelativePath", [](const Task *, const QString &) {
        auto buffer = getCurrentBuffer();
        if (buffer) {
            auto node = buffer->getNode();
            if (node) {
                return PathUtils::cleanPath(node->fetchPath());
            } else {
                return PathUtils::cleanPath(buffer->getPath());
            }
        }
        return QString();
    });
    addVariable("bufferName", [](const Task *, const QString &) {
        auto buffer = getCurrentBuffer();
        if (buffer) {
            return PathUtils::fileName(buffer->getPath());
        }
        return QString();
    });
    addVariable("bufferBaseName", [](const Task *, const QString &) {
        auto buffer = getCurrentBuffer();
        if (buffer) {
            return QFileInfo(buffer->getPath()).completeBaseName();
        }
        return QString();
    });
    addVariable("bufferDir", [](const Task *, const QString &) {
        auto buffer = getCurrentBuffer();
        if (buffer) {
            return PathUtils::parentDirPath(buffer->getPath());
        }
        return QString();
    });
    addVariable("bufferExt", [](const Task *, const QString &) {
        auto buffer = getCurrentBuffer();
        if (buffer) {
            return QFileInfo(buffer->getPath()).suffix();
        }
        return QString();
    });
    addVariable("selectedText", [](const Task *, const QString &) {
        auto win = getCurrentViewWindow();
        if (win) {
            return win->selectedText();
        }
        return QString();
    });
}

void TaskVariableMgr::initTaskVariables()
{
    addVariable("cwd", [](const Task *p_task, const QString &) {
        return PathUtils::cleanPath(p_task->getOptionsCwd());
    });
    addVariable("taskFile", [](const Task *p_task, const QString &) {
        return PathUtils::cleanPath(p_task->getFile());
    });
    addVariable("taskDir", [](const Task *p_task, const QString &) {
        return PathUtils::parentDirPath(p_task->getFile());
    });
    addVariable("exeFile", [](const Task *, const QString &) {
        return PathUtils::cleanPath(qApp->applicationFilePath());
    });
    addVariable("pathSeparator", [](const Task *, const QString &) {
        return QDir::separator();
    });
    addVariable("notebookTaskFolder", [this](const Task *, const QString &) {
        return PathUtils::cleanPath(m_taskMgr->getNotebookTaskFolder());
    });
    addVariable("userTaskFolder", [](const Task *, const QString &) {
        return PathUtils::cleanPath(ConfigMgr::getInst().getUserTaskFolder());
    });
    addVariable("appTaskFolder", [](const Task *, const QString &) {
        return PathUtils::cleanPath(ConfigMgr::getInst().getAppTaskFolder());
    });
    addVariable("userThemeFolder", [](const Task *, const QString &) {
        return PathUtils::cleanPath(ConfigMgr::getInst().getUserThemeFolder());
    });
    addVariable("appThemeFolder", [](const Task *, const QString &) {
        return PathUtils::cleanPath(ConfigMgr::getInst().getAppThemeFolder());
    });
    addVariable("userDocsFolder", [](const Task *, const QString &) {
        return PathUtils::cleanPath(ConfigMgr::getInst().getUserDocsFolder());
    });
    addVariable("appDocsFolder", [](const Task *, const QString &) {
        return PathUtils::cleanPath(ConfigMgr::getInst().getAppDocsFolder());
    });
}

void TaskVariableMgr::addVariable(const QString &p_name, const TaskVariable::Func &p_func)
{
    Q_ASSERT(!m_variables.contains(p_name));

    m_variables.insert(p_name, TaskVariable(p_name, p_func));
}

const ViewWindow *TaskVariableMgr::getCurrentViewWindow()
{
    return VNoteX::getInst().getMainWindow()->getViewArea()->getCurrentViewWindow();
}

Buffer *TaskVariableMgr::getCurrentBuffer()
{
    auto win = getCurrentViewWindow();
    if (win) {
        return win->getBuffer();
    }
    return nullptr;
}

QSharedPointer<Notebook> TaskVariableMgr::getCurrentNotebook()
{
    return VNoteX::getInst().getNotebookMgr().getCurrentNotebook();
}

QString TaskVariableMgr::evaluate(const Task *p_task, const QString &p_text) const
{
    QString content(p_text);

    int maxTimesAtSamePos = 100;

    QRegularExpression regExp(c_variableSymbolRegExp);
    int pos = 0;
    while (pos < content.size()) {
        QRegularExpressionMatch match;
        int idx = content.indexOf(regExp, pos, &match);
        if (idx == -1) {
            break;
        }

        const auto varName = match.captured(1).trimmed();
        const auto varValue = match.captured(2).trimmed();
        auto var = findVariable(varName);
        if (!var) {
            // Skip it.
            pos = idx + match.capturedLength(0);
            continue;
        }

        const auto afterText = var->evaluate(p_task, varValue);
        content.replace(idx, match.capturedLength(0), afterText);

        // @afterText may still contains variable symbol.
        if (pos == idx) {
            if (--maxTimesAtSamePos == 0) {
                break;
            }
        } else {
            maxTimesAtSamePos = 100;
        }
        pos = idx;
    }

    return content;
}

    /*
    auto text = p_text;
    auto eval = [&text](const QString &p_name, std::function<QString()> p_func) {
        auto reg = QRegularExpression(QString(R"(\$\{[\t ]*%1[\t ]*\})").arg(p_name));
        if (text.contains(reg)) {
            text.replace(reg, p_func());
        }
    };

    // Magic variables
    {
        auto cDT = QDateTime::currentDateTime();
        for(auto s : {
            "d", "dd", "ddd", "dddd", "M", "MM", "MMM", "MMMM",
            "yy", "yyyy", "h", "hh", "H", "HH", "m", "mm",
            "s", "ss", "z", "zzz", "AP", "A", "ap", "a"
        }) eval(QString("magic:%1").arg(s), [s]() {
            return QDateTime::currentDateTime().toString(s);
        });
        eval("magic:random", []() {
            return QString::number(QRandomGenerator::global()->generate());
        });
        eval("magic:random_d", []() {
            return QString::number(QRandomGenerator::global()->generate());
        });
        eval("magic:date", []() {
            return QDate::currentDate().toString("yyyy-MM-dd");
        });
        eval("magic:da", []() {
            return QDate::currentDate().toString("yyyyMMdd");
        });
        eval("magic:time", []() {
            return QTime::currentTime().toString("hh:mm:ss");
        });
        eval("magic:datetime", []() {
            return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        });
        eval("magic:dt", []() {
            return QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss");
        });
        eval("magic:note", []() {
            auto file = TaskHelper::getCurrentBufferPath();
            return QFileInfo(file).fileName();
        });
        eval("magic:no", []() {
            auto file = TaskHelper::getCurrentBufferPath();
            return QFileInfo(file).completeBaseName();
        });
        eval("magic:t", []() {
            auto dt = QDateTime::currentDateTime();
            return dt.timeZone().displayName(dt, QTimeZone::ShortName);
        });
        eval("magic:w", []() {
            return QString::number(QDate::currentDate().weekNumber());
        });
        eval("magic:att", []() {
            // TODO
            return QString();
        });
    }

    // environment variables
    do {
        QMap<QString, QString> map;
        auto list = TaskHelper::getAllSpecialVariables("env", p_text);
        list.erase(std::unique(list.begin(), list.end()), list.end());
        if (list.isEmpty()) break;
        for (const auto &name : list) {
            auto value = QProcessEnvironment::systemEnvironment().value(name);
            map.insert(name, value);
        }
        text = TaskHelper::replaceAllSepcialVariables("env", text, map);
    } while(0);

    // config variables
    do {
        const auto config_obj = ConfigMgr::getInst().getConfig().toJson();
        QMap<QString, QString> map;
        auto list = TaskHelper::getAllSpecialVariables("config", p_text);
        if (list.isEmpty()) break;
        list.erase(std::unique(list.begin(), list.end()), list.end());
        for (const auto &name : list) {
            auto value = TaskHelper::evaluateJsonExpr(config_obj, name);
            qDebug() << "insert" << name << value;
            map.insert(name, value);
        }
        text = TaskHelper::replaceAllSepcialVariables("config", text, map);
    } while(0);

    // input variables
    text = evaluateInputVariables(text, p_task);
    // shell variables
    text = evaluateShellVariables(text, p_task);
    return text;
    */

QStringList TaskVariableMgr::evaluate(const Task *p_task, const QStringList &p_texts) const
{
    QStringList strs;
    for (const auto &str : p_texts) {
        strs << evaluate(p_task, str);
    }
    return strs;
}

/*
QString TaskVariableMgr::evaluateInputVariables(const QString &p_text,
                                                const Task *p_task) const
{
    QMap<QString, QString> map;
    auto list = TaskHelper::getAllSpecialVariables("input", p_text);
    list.erase(std::unique(list.begin(), list.end()), list.end());
    if (list.isEmpty()) return p_text;
    for (const auto &id : list) {
        auto input = p_task->getInput(id);
        QString text;
        auto mainwin = VNoteX::getInst().getMainWindow();
        if (input.type == "promptString") {
            auto desc = evaluate(input.description, p_task);
            auto defaultText = evaluate(input.default_, p_task);
            QInputDialog dialog(mainwin);
            dialog.setInputMode(QInputDialog::TextInput);
            if (input.password) dialog.setTextEchoMode(QLineEdit::Password);
            else dialog.setTextEchoMode(QLineEdit::Normal);
            dialog.setWindowTitle(p_task->getLabel());
            dialog.setLabelText(desc);
            dialog.setTextValue(defaultText);
            if (dialog.exec() == QDialog::Accepted) {
                text = dialog.textValue();
            } else {
                throw "TaskCancle";
            }
        } else if (input.type == "pickString") {
            // TODO: select description
            SelectDialog dialog(p_task->getLabel(), input.description, mainwin);
            for (int i = 0; i < input.options.size(); i++) {
                dialog.addSelection(input.options.at(i), i);
            }

            if (dialog.exec() == QDialog::Accepted) {
                int selection = dialog.getSelection();
                text = input.options.at(selection);
            } else {
                throw "TaskCancle";
            }
        }
        map.insert(input.id, text);
    }
    return TaskHelper::replaceAllSepcialVariables("input", p_text, map);
}

QString TaskVariableMgr::evaluateShellVariables(const QString &p_text,
                                                const Task *p_task) const
{
    QMap<QString, QString> map;
    auto list = TaskHelper::getAllSpecialVariables("shell", p_text);
    list.erase(std::unique(list.begin(), list.end()), list.end());
    if (list.isEmpty()) return p_text;
    for (const auto &cmd : list) {
        QProcess process;
        process.setWorkingDirectory(p_task->getOptionsCwd());
        ShellExecution::setupProcess(&process, cmd);
        process.start();
        if (!process.waitForStarted(1000) || !process.waitForFinished(1000)) {
            throw "Shell variable execution timeout";
        }
        auto res = process.readAllStandardOutput().trimmed();
        map.insert(cmd, res);
    }
    return TaskHelper::replaceAllSepcialVariables("shell", p_text, map);
}
*/

const TaskVariable *TaskVariableMgr::findVariable(const QString &p_name) const
{
    auto it = m_variables.find(p_name);
    if (it != m_variables.end()) {
        return &(it.value());
    }

    return nullptr;
}

void TaskVariableMgr::overrideVariable(const QString &p_name, const TaskVariable::Func &p_func)
{
    m_variables.insert(p_name, TaskVariable(p_name, p_func));
}
