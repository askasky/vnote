#include "test_task.h"

#include <QDebug>

#include <task/taskvariablemgr.h>
#include <task/task.h>

using namespace tests;

using namespace vnotex;

TestTask::TestTask(QObject *p_parent)
    : QObject(p_parent)
{
}

void TestTask::TestTaskVariableMgr()
{
    TaskVariableMgr mgr(nullptr);
    mgr.init();

    mgr.overrideVariable("notebookFolder", [](const Task *, const QString &val) {
        Q_ASSERT(val.isEmpty());
        return "/home/vnotex/vnote";
    });

    mgr.overrideVariable("notebookFolderName", [](const Task *, const QString &val) {
        Q_ASSERT(val.isEmpty());
        return "vnote";
    });

    auto task = createTask();

    auto result = mgr.evaluate(task.data(), "start ${notebookFolder} end");
    QCOMPARE("start /home/vnotex/vnote end", result);

    result = mgr.evaluate(task.data(), "start ${notebookFolder} mid ${notebookFolderName} end");
    QCOMPARE("start /home/vnotex/vnote mid vnote end", result);
}

QSharedPointer<vnotex::Task> TestTask::createTask() const
{
    return QSharedPointer<Task>(new Task("en_US", "dummy_file", nullptr, nullptr));
}

QTEST_MAIN(tests::TestTask)

