/*
 * Copyright (C) 2010 Igalia S.L.
 *
 * Contact: Iago Toral Quiroga <itoral@igalia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "ms-media-source.h"
#include "ms-metadata-source-priv.h"

#include <string.h>

#define MS_MEDIA_SOURCE_GET_PRIVATE(object)				\
  (G_TYPE_INSTANCE_GET_PRIVATE((object), MS_TYPE_MEDIA_SOURCE, MsMediaSourcePrivate))

struct _MsMediaSourcePrivate {
  guint padding;
};

struct FullResolutionCtlCb {
  MsMediaSourceResultCb user_callback;
  gpointer user_data;
  GList *source_map_list;
};

struct FullResolutionDoneCb {
  MsMediaSourceResultCb user_callback;
  gpointer user_data;
  guint pending_callbacks;
  MsMediaSource *source;
  guint browse_id;
  guint remaining;
};

static void
ms_media_source_full_resolution_done_cb (MsMetadataSource *source,
					 MsContent *media,
					 gpointer user_data,
					 const GError *error)
{
  g_debug ("media_source_full_resolution_done_cb");

  struct FullResolutionDoneCb *cb_info = 
    (struct FullResolutionDoneCb *) user_data;

  cb_info->pending_callbacks--;

  if (error) {
    g_warning ("Failed to fully resolve some metadata: %s", error->message);
  }

  if (cb_info->pending_callbacks == 0) {
    cb_info->user_callback (cb_info->source, 
			    cb_info->browse_id, 
			    media,
			    cb_info->remaining, 
			    cb_info->user_data,
			    NULL);
  }
}

static void
ms_media_source_full_resolution_ctl_cb (MsMediaSource *source,
					guint browse_id,
					MsContent *media,
					guint remaining,
					gpointer user_data,
					const GError *error)
{
  GList *iter;

  struct FullResolutionCtlCb *ctl_info =
    (struct FullResolutionCtlCb *) user_data;

  g_debug ("media_source_full_resolution_ctl_cb");

  /* If we got an error, invoke the user callback right away and bail out */
  if (error) {
    g_warning ("Operation failed: %s", error->message);
    ctl_info->user_callback (source,
			     browse_id,
			     media,
			     remaining,
			     ctl_info->user_data,
			     error);
    return;
  }

  /* Save all the data we need to emit the result */
  struct FullResolutionDoneCb *done_info =
    g_new (struct FullResolutionDoneCb, 1);
  done_info->user_callback = ctl_info->user_callback;
  done_info->user_data = ctl_info->user_data;
  done_info->pending_callbacks = g_list_length (ctl_info->source_map_list);
  done_info->source = source;
  done_info->browse_id = browse_id;
  done_info->remaining = remaining;

  /* Use sources in the map to fill in missing metadata, the "done"
     callback will be used to emit the resulting object when 
     all metadata has been gathered */
  iter = ctl_info->source_map_list;
  while (iter) {
    gchar *name;
    struct SourceKeyMap *map = (struct SourceKeyMap *) iter->data;
    g_object_get (map->source, "source-name", &name, NULL);
    g_debug ("Using '%s' to resolve extra metadata now", name);

    ms_metadata_source_resolve (map->source, 
			     map->keys, 
			     media, 
			     ms_media_source_full_resolution_done_cb,
			     done_info);

    iter = g_list_next (iter);
  }
}

G_DEFINE_ABSTRACT_TYPE (MsMediaSource, ms_media_source, MS_TYPE_METADATA_SOURCE);

static void
ms_media_source_class_init (MsMediaSourceClass *media_source_class)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (media_source_class);

  g_type_class_add_private (media_source_class, sizeof (MsMediaSourcePrivate));
}

static void
ms_media_source_init (MsMediaSource *source)
{
  source->priv = MS_MEDIA_SOURCE_GET_PRIVATE (source);
  memset (source->priv, 0, sizeof (MsMediaSourcePrivate));
}

guint
ms_media_source_browse (MsMediaSource *source, 
			const gchar *container_id,
			const GList *keys,
			guint skip,
			guint count,
			guint flags,
			MsMediaSourceResultCb callback,
			gpointer user_data)
{
  MsMediaSourceResultCb _callback;
  gpointer _user_data ;
  GList *_keys;
  struct SourceKeyMapList key_mapping;
  
  /* By default assume we will use the parameters specified by the user */
  _keys = (GList *) keys;
  _callback = callback;
  _user_data = user_data;

  if (flags & MS_METADATA_RESOLUTION_FULL) {
    g_debug ("requested full browse");
    ms_metadata_source_setup_full_resolution_mode (MS_METADATA_SOURCE (source),
						   keys, &key_mapping);

    /* If we do not have a source map for the unsupported keys then
       we cannot resolve any of them */
    if (key_mapping.source_maps != NULL) {
      struct FullResolutionCtlCb *c = g_new0 (struct FullResolutionCtlCb, 1);
      c->user_callback = callback;
      c->user_data = user_data;
      c->source_map_list = key_mapping.source_maps;
      
      _callback = ms_media_source_full_resolution_ctl_cb;
      _user_data = c;
      _keys = key_mapping.operation_keys;
    }    
  }

  return MS_MEDIA_SOURCE_GET_CLASS (source)->browse (source,
                                                     container_id,
                                                     _keys,
                                                     skip, count,
                                                     _callback, _user_data);
}

guint
ms_media_source_search (MsMediaSource *source,
                        const gchar *text,
                        const GList *keys,
                        const gchar *filter,
                        guint skip,
                        guint count,
                        guint flags,
                        MsMediaSourceResultCb callback,
                        gpointer user_data)
{
  MsMediaSourceResultCb _callback;
  gpointer _user_data ;
  GList *_keys;
  struct SourceKeyMapList key_mapping;
  
  /* By default assume we will use the parameters specified by the user */
  _callback = callback;
  _user_data = user_data;
  _keys = (GList *) keys;

  if (flags & MS_METADATA_RESOLUTION_FULL) {
    g_debug ("requested full search");
    ms_metadata_source_setup_full_resolution_mode (MS_METADATA_SOURCE (source),
						   keys, &key_mapping);
    
    /* If we do not have a source map for the unsupported keys then
       we cannot resolve any of them */
    if (key_mapping.source_maps != NULL) {
      struct FullResolutionCtlCb *c = g_new0 (struct FullResolutionCtlCb, 1);
      c->user_callback = callback;
      c->user_data = user_data;
      c->source_map_list = key_mapping.source_maps;
      
      _callback = ms_media_source_full_resolution_ctl_cb;
      _user_data = c;
      _keys = key_mapping.operation_keys;
    }    
  }

  return MS_MEDIA_SOURCE_GET_CLASS (source)->search (source,
                                                     text,
                                                     _keys,
                                                     filter,
                                                     skip, count,
                                                     _callback, _user_data);
}
