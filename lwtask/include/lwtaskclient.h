/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwtaskclient.h
 *
 * Abstract:
 *
 *        Likewise Task System (LWTASK)
 *
 *        Client API
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#ifndef __LW_TASK_CLIENT_H__
#define __LW_TASK_CLIENT_H__

typedef struct _LW_TASK_CLIENT_CONNECTION* PLW_TASK_CLIENT_CONNECTION;

typedef struct _LW_TASK_STATUS
{
    DWORD  dwError;

    double percentComplete;

} LW_TASK_STATUS, *PLW_TASK_STATUS;

typedef struct _LW_TASK_ARG
{
    PSTR pszArgName;
    PSTR pszArgValue;

} LW_TASK_ARG, *PLW_TASK_ARG;

typedef struct _LW_TASK_ARG_INFO
{
    PSTR             pszArgName;
    LW_TASK_ARG_TYPE argType;
    LW_TASK_ARG_FLAG dwFlags;

} LW_TASK_ARG_INFO, *PLW_TASK_ARG_INFO;

typedef struct _LW_TASK_INFO
{
    PSTR             pszTaskId;
    PLW_TASK_ARG     pArgArray;
    DWORD            dwNumArgs;
} LW_TASK_INFO, *PLW_TASK_INFO;

DWORD
LwTaskOpenServer(
    PLW_TASK_CLIENT_CONNECTION* ppConnection
    );

DWORD
LwTaskGetTypes(
    LW_TASK_TYPE* pTaskTypes,
    PDWORD        pdwNumTypes
    );

DWORD
LwTaskGetSchema(
    LW_TASK_TYPE       taskType,
    PLW_TASK_ARG_INFO* ppArgInfoArray,
    PDWORD             pdwNumArgInfo
    );

DWORD
LwTaskEnum(
    LW_TASK_TYPE   taskType,
    PLW_TASK_INFO* pTaskInfoArray,
    PDWORD         pdwNumTaskInfos,
    PDWORD         pdwResume
    );

DWORD
LwTaskCreate(
    LW_TASK_TYPE taskType,
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    );

DWORD
LwTaskStart(
    PCSTR pszTaskId
    );

DWORD
LwTaskGetStatus(
    PCSTR           pszTaskId,
    PLW_TASK_STATUS pStatus
    );

DWORD
LwTaskStop(
    PCSTR pszTaskId
    );

DWORD
LwTaskDelete(
    PCSTR pszTaskId
    );

VOID
LwTaskFreeTaskInfoArray(
    PLW_TASK_INFO pTaskInfoArray,
    DWORD         dwNumTaskInfos
    );

VOID
LwTaskFreeArgInfoArray(
    PLW_TASK_ARG_INFO pArgInfoArray,
    DWORD             dwNumArgInfos
    );

DWORD
LwTaskSetLogLevel(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    LW_TASK_LOG_LEVEL          logLevel
    );

DWORD
LwTaskSetLogInfo(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PLW_TASK_LOG_INFO          pLogInfo
    );

DWORD
LwTaskGetLogInfo(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    PLW_TASK_LOG_INFO*         ppLogInfo
    );

DWORD
LwTaskGetPid(
    PLW_TASK_CLIENT_CONNECTION pConnection,
    pid_t*                     pPid
    );

DWORD
LwTaskCloseServer(
    PLW_TASK_CLIENT_CONNECTION pConnection
    );

#endif /* __LW_TASK_CLIENT_H__ */

