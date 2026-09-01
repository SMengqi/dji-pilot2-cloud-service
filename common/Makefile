# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose


PSROOT = ..
TARGET = libcom_$(BUILDCOMMON).a
include ../build_scripts/platform.mk

OBJECT +=  decl_module.o
OBJECT +=  decl_event.o
OBJECT +=  decl_errid.o
OBJECT +=  decl_alarmid.o
OBJECT +=  json2pb/json2pb.o
OBJECT +=  if/pl_utils.o
OBJECT +=  if/messageDispatcher.o
OBJECT +=  if/requestContextManager.o
OBJECT +=  thread/ThreadPoolManager.o
OBJECT +=  if/baseModule.o
OBJECT +=  hdmap/pf_map_block_3d.o

OBJECT +=  proto/if_proto_common.pb.o
OBJECT +=  proto/if_drc_drsu.pb.o
OBJECT +=  proto/bxt.obstacle_v2.pb.o
OBJECT +=  proto/bxt.common.Header.pb.o
OBJECT +=  proto/bxt.common.math.MatrixXd.pb.o
OBJECT +=  proto/bxt.common.sensor.SensorSource.pb.o
OBJECT +=  proto/bxt.common.math.Vector3d.pb.o
OBJECT +=  proto/bxt.StatusPb.pb.o
OBJECT +=  proto/bxt.common.math.point.pb.o
OBJECT +=  proto/if_drsu_geofencing.pb.o
OBJECT +=  proto/drc_event.pb.o
OBJECT +=  proto/log_level.pb.o
OBJECT +=  proto/thread_cpucore_cfg.pb.o

OBJECT +=  proto/kafka_config.pb.o
OBJECT +=  proto/bxt_mqtt_config.pb.o
OBJECT +=  proto/bxt_if_cns.pb.o
OBJECT +=  proto/bxt.uav.pb.o
OBJECT +=  proto/local_cfg.pb.o
OBJECT +=  proto/bxt_if_uav.pb.o
OBJECT +=  proto/dji_cloud_api.pb.o
OBJECT +=  proto/bxt_cloud_common.pb.o
OBJECT +=  proto/dji_dock3_osd.pb.o

#OBJECT +=  proto/http_client_cfg.pb.o
#OBJECT +=  proto/bxt_if_ladar.pb.o
#OBJECT +=  proto/cmlc_comm_5ga.pb.o
#OBJECT +=  proto/sense_data_5g_a.pb.o

# OBJECT +=  proto/if_drc_fusion.pb.o
# OBJECT +=  proto/config_file.pb.o
# OBJECT +=  proto/bxt.PointUTM.pb.o
# OBJECT +=  proto/pad_msg.pb.o
# OBJECT +=  proto/chassis.pb.o
# OBJECT +=  proto/control_command.pb.o
# OBJECT +=  proto/drc_log_config.pb.o
# OBJECT +=  hdmap/pf_map_block.o
# OBJECT +=  proto/drc_event_debug.pb.o
# OBJECT +=  proto/if_platform_stream.pb.o
# OBJECT +=  proto/if_drc_crm_r4.pb.o
# OBJECT +=  proto/if_drc_acu.pb.o
# OBJECT +=  proto/if_broadxt_zebra.pb.o
# OBJECT +=  proto/bxt_cns_id.pb.o
# OBJECT +=  proto/cns_nacos_config.pb.o

include ../build_scripts/staticlibrary.mk
