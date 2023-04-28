﻿#include <qobject.h>
#include <qstring.h>
#include <pluginGl.h>
#include <functional>
#include <qsciscintilla.h>
#include <QAction>
#include <QDebug>

#include "actorprocessor.h"
#include "plugin.h"

#define NDD_EXPORTDLL

#if defined(Q_OS_WIN)
#if defined(NDD_EXPORTDLL)
#define NDD_EXPORT __declspec(dllexport)
#else
#define NDD_EXPORT __declspec(dllimport)
#endif
#else
#define NDD_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

NDD_EXPORT bool NDD_PROC_IDENTIFY(NDD_PROC_DATA* pProcData);
//NDD_EXPORT int NDD_PROC_MAIN(QWidget* pNotepad, const QString& strFileName, std::function<QsciScintilla* ()>getCurEdit, NDD_PROC_DATA* procData);
NDD_EXPORT int NDD_PROC_MAIN_V2(QWidget* pNotepad, const QString& strFileName,ActorProcessor* processor, NDD_PROC_DATA* procData);

#ifdef __cplusplus
}
#endif

static NDD_PROC_DATA s_procData;
static QWidget* s_pMainNotepad = nullptr;
std::function<QsciScintilla* ()> s_getCurEdit;

bool NDD_PROC_IDENTIFY(NDD_PROC_DATA* pProcData)
{
    if(pProcData == NULL)
    {
        return false;
    }
    pProcData->m_strPlugName = QObject::tr("between-plugin");
    pProcData->m_strComment = QObject::tr("test");

    pProcData->m_version = QString("1.0");
    pProcData->m_auther = QString("matheuter@gmail.com");

    pProcData->m_menuType = 1;
    return true;
}

//测试插件之间通信
int NDD_PROC_MAIN_V2(QWidget* pNotepad, const QString& strFileName,ActorProcessor* processor,NDD_PROC_DATA* pProcData)
{
    //务必拷贝一份pProcData，在外面会释放。
    if (pProcData != nullptr)
    {
        s_procData = *pProcData;
    }
    else
    {
        return -1;
    }

    s_pMainNotepad = pNotepad;

    Plugin * plugin = new Plugin();

    plugin->setActorProcessor(processor);
    plugin->setTopMenu(pProcData->m_rootMenu);

    return 0;
}
