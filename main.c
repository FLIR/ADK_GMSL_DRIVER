/* NVIDIA CORPORATION gave permission to FLIR Systems, Inc to modify this code
  * and distribute it as part of the ADAS GMSL Kit.
  * http://www.flir.com/
  * October-2019
*/
/* Copyright (c) 2015-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <stdio.h>
#include <signal.h>

#include "main.h"
#include "check_version.h"
#include "capture.h"
#include "save.h"
#include "display.h"

/* Quit flag. Out of context structure for sig handling */
static volatile NvMediaBool *quit_flag;
static char *cmd_listener;

static void
SigHandler(int signum)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGKILL, SIG_IGN);
    signal(SIGSTOP, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    *quit_flag = NVMEDIA_TRUE;

    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGKILL, SIG_DFL);
    signal(SIGSTOP, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
}

static void
SigSetup(void)
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = SigHandler;

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGKILL, &action, NULL);
    sigaction(SIGSTOP, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
}
static int
ExecuteNextCommand(NvMainContext *ctx) {
    char input[256] = { 0 };

    if (!fgets(input, 256, stdin)) {
        if(*quit_flag != NVMEDIA_TRUE) {
            LOG_ERR("%s: Failed to read command\n", __func__);
        }
    }

    /* Remove new line character */
    if (input[strlen(input) - 1] == '\n')
        input[strlen(input) - 1] = '\0';

    if (!strcasecmp(input, "q") || !strcasecmp(input, "quit")) {
        *quit_flag = NVMEDIA_TRUE;
        return 0;
    } else if(input[0] != '\0') {
        sprintf(cmd_listener, input);
    }

    return 0;
}

static NvMediaStatus 
InitSignals(NvMainContext *mainCtx) {
    sigset_t set;
    int status;

    /* prepare an empty signal set */
    status = sigemptyset(&set);

    if (0 != status) {
        LOG_ERR("%s: Failed to empty signal set\n");
        return NVMEDIA_STATUS_ERROR;
    }

    /* add signals to be blocked */
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGKILL);
    sigaddset(&set, SIGSTOP);
    sigaddset(&set, SIGHUP);

    /* block all signals, the new threads created will inherit the signal mask */
    status = pthread_sigmask(SIG_BLOCK, &set, NULL);

    if (0 != status) {
        LOG_ERR("%s: Failed to blocks signals\n");
        return NVMEDIA_STATUS_ERROR;
    }

    memset(mainCtx, 0, sizeof(NvMainContext));
    mainCtx->cmd = malloc(256);
    strcpy(mainCtx->cmd, "");

    if (CheckModulesVersion() != NVMEDIA_STATUS_OK) {
        return NVMEDIA_STATUS_ERROR;
    }

    quit_flag = &mainCtx->quit;
    cmd_listener = mainCtx->cmd;
    SigSetup();

    return NVMEDIA_STATUS_OK;
}

static NvMediaStatus
InitPipeline(NvMainContext *mainCtx, TestArgs *allArgs) {
    /* Initialize context */
    mainCtx->testArgs = allArgs;

    /* Initialize all the components */
    if (CaptureInit(mainCtx) != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to Initialize Capture\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    if (SaveInit(mainCtx) != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to Initialize Save\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    if (DisplayInit(mainCtx) != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: Failed to Initialize Display\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    /* Call Proc for each component */
    if (CaptureProc(mainCtx) != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: CaptureProc Failed\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    if (SaveProc(mainCtx) != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: SaveProc Failed\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    if (DisplayProc(mainCtx) != NVMEDIA_STATUS_OK) {
        LOG_ERR("%s: DisplayProc Failed\n", __func__);
        return NVMEDIA_STATUS_ERROR;
    }

    return NVMEDIA_STATUS_OK;
}

static NvMediaStatus
InitRunner(NvMainContext *mainCtx, TestArgs *allArgs) {
    NvMediaStatus sigResult = InitSignals(mainCtx);
    if(sigResult != NVMEDIA_STATUS_OK) {
        return sigResult;
    }
    return InitPipeline(mainCtx, allArgs);
}

int Run(TestArgs *allArgs, NvMainContext *mainCtx)
{
    if(InitRunner(mainCtx, allArgs) != NVMEDIA_STATUS_OK) {
        goto done;
    }

    while (!mainCtx->quit) {
        if (!allArgs->frames.isUsed) {
            ExecuteNextCommand(mainCtx);
        }
    }

done:
    DisplayFini(mainCtx);
    SaveFini(mainCtx);
    CaptureFini(mainCtx);
    return 0;
}
