/*
 * Copyright (c) 2018
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "discovery.h"
#include "service.h"
#include "profiles.h"
#include "metadata.h"
#include "edgex-rest.h"

#include <string.h>
#include <stdlib.h>

#include <microhttpd.h>

static void *edgex_device_handler_do_discovery (void *p)
{
  devsdk_service_t *svc = (devsdk_service_t *) p;

  pthread_mutex_lock (&svc->discolock);
  svc->userfns.discover (svc->userdata);
  pthread_mutex_unlock (&svc->discolock);
  return NULL;
}

int edgex_device_handler_discovery
(
  void *ctx,
  char *url,
  const devsdk_nvpairs *qparams,
  edgex_http_method method,
  const char *upload_data,
  size_t upload_data_size,
  void **reply,
  size_t *reply_size,
  const char **reply_type
)
{
  devsdk_service_t *svc = (devsdk_service_t *) ctx;

  if (svc->userfns.discover == NULL)
  {
    return MHD_HTTP_NOT_IMPLEMENTED;
  }

  if (svc->adminstate == LOCKED || svc->opstate == DISABLED)
  {
    return MHD_HTTP_LOCKED;
  }

  if (!svc->config.device.discovery)
  {
    *reply = strdup ("Discovery disabled by configuration\n");
    *reply_size = strlen (*reply);
    return MHD_HTTP_SERVICE_UNAVAILABLE;
  }

  if (pthread_mutex_trylock (&svc->discolock) == 0)
  {
    iot_threadpool_add_work (svc->thpool, edgex_device_handler_do_discovery, svc, -1);
    pthread_mutex_unlock (&svc->discolock);
  }
  // else discovery was already running; ignore this request

  *reply = strdup ("Running discovery\n");
  *reply_size = strlen (*reply);
  return MHD_HTTP_OK;
}
